#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <android/log.h>
#include <cmath>
#include <cstring>
#include <jni.h>
#include <memory.h>
#include <stdlib.h>
#include <sys/time.h>

#include "HIAIMixModel.h"

#define LOG_TAG "SYNC_DDK_MSG"

#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

static HIAI_MixModelManager *manager = NULL;
static HIAI_MixTensorBuffer *inputTensors = NULL;
static HIAI_MixTensorBuffer *outputTensors = NULL;
static HIAI_MixModelBuffer *modelBuffer = NULL;
static HIAI_MixModelTensorInfo *modelTensorinfo = NULL;

int outputSize = 0;

extern "C" JNIEXPORT jint JNICALL
Java_com_example_hiai_ModelManager_loadModelSync(
    JNIEnv *env, jclass instance, jstring jmodelName, jstring jofflinemodelfile,
    jobject assetManager, jboolean isMixModel) {
    const char *modelName = env->GetStringUTFChars(jmodelName, 0);
    const char *offlinemodelfile = env->GetStringUTFChars(jofflinemodelfile, 0);

    manager = HIAI_MixModelManager_Create(NULL);
    if (manager == NULL) {
        return -1;
    }

    AAssetManager *mgr = AAssetManager_fromJava(env, assetManager);
    LOGI("Attempting to load model...\n");
    LOGI("model name is %s", modelName);
    LOGI("offlinemodelfile is %s", offlinemodelfile);
    AAsset *asset =
        AAssetManager_open(mgr, offlinemodelfile, AASSET_MODE_BUFFER);

    if (nullptr == asset) {
        LOGE("AAsset is null...\n");
        return -1;
    }

    const void *data = AAsset_getBuffer(asset);
    if (nullptr == data) {
        LOGE("model buffer is null...\n");
        return -1;
    }

    off_t len = AAsset_getLength(asset);
    if (0 == len) {
        LOGE("model buffer length is 0...\n");
        return -1;
    }

    modelBuffer = HIAI_MixModelBuffer_Create_From_Buffer(
        modelName, (void *)data, len, HIAI_MixDevPerf::HIAI_MIX_DEVPREF_HIGH,
        isMixModel);
    if (modelBuffer == NULL) {
        LOGE("failed to get the modelbuffer\n");
        HIAI_MixModelManager_Destroy(manager);
        return -1;
    }

    HIAI_MixModelBuffer *bufferarray[1] = {modelBuffer};
    int ret = HIAI_MixModel_LoadFromModelBuffers(manager, bufferarray, 1);
    if (ret == 0) {
        // get tensor information
        modelTensorinfo = HIAI_MixModel_GetModelTensorInfo(manager, modelName);
    }
    LOGI("load model from assets ret = %d", ret);

    env->ReleaseStringUTFChars(jmodelName, modelName);
    env->ReleaseStringUTFChars(jofflinemodelfile, offlinemodelfile);
    AAsset_close(asset);

    return ret;
}

extern "C" JNIEXPORT jint JNICALL
Java_com_example_hiai_ModelManager_unloadModelSync(JNIEnv *env,
                                                              jclass instance) {
    if (NULL == manager) {
        LOGE("please load model first.");
        return -1;
    }
    if (modelBuffer != NULL) {
        HIAI_MixModelBuffer_Destroy(modelBuffer);
        modelBuffer = NULL;
    }
    if (NULL != modelTensorinfo) {
        HIAI_MixModel_ReleaseModelTensorInfo(modelTensorinfo);
    }

    int ret = HIAI_MixModel_UnLoadModel(manager);

    LOGI("JNI unload model ret:%d", ret);

    HIAI_MixModelManager_Destroy(manager);
    manager = NULL;

    return ret;
}

extern "C" JNIEXPORT jint JNICALL
Java_com_example_hiai_ModelManager_loadModelFromFileSync(
    JNIEnv *env, jclass type, jstring jmodelName, jstring jmodelPath,
    jboolean isMixModel) {
    const char *modelName = env->GetStringUTFChars(jmodelName, 0);
    const char *modelPath = env->GetStringUTFChars(jmodelPath, 0);
    LOGI("Java_com_example_hiai_ModelManager_"
         "loadModelFromFileSync. "
         "modelPath : %s ",
         modelPath);
    LOGI("Java_com_example_hiai_ModelManager_"
         "loadModelFromFileSync. "
         "modelName : %s ",
         modelName);

    modelBuffer = HIAI_MixModelBuffer_Create_From_File(
        modelName, modelPath, HIAI_MIX_DEVPREF_LOW, isMixModel);
    if (modelBuffer == NULL) {
        LOGE("faild to create mixModelBuffer");
        return -1;
    }

    HIAI_MixModelBuffer *modelBufferArray[] = {modelBuffer};
    manager = HIAI_MixModelManager_Create(NULL);
    if (manager == NULL) {
        LOGE("failed to create HIAI_MixModelManager");
        return -1;
    }

    int ret = HIAI_MixModel_LoadFromModelBuffers(manager, modelBufferArray, 1);

    LOGI("load model from file ret = %d", ret);
    if (ret == 0) {
        modelTensorinfo = HIAI_MixModel_GetModelTensorInfo(manager, modelName);
    }

    env->ReleaseStringUTFChars(jmodelName, modelName);
    env->ReleaseStringUTFChars(jmodelPath, modelPath);
    return ret;
}

extern "C" JNIEXPORT jint JNICALL
Java_com_example_hiai_ModelManager_loadModelSyncFromBuffer(
    JNIEnv *env, jclass type, jstring offlineModelName_,
    jbyteArray offlineModelBuffer_, jboolean isMixModel) {
    const char *modelName = env->GetStringUTFChars(offlineModelName_, 0);
    jbyte *offlineModelBuffer =
        env->GetByteArrayElements(offlineModelBuffer_, NULL);

    manager = HIAI_MixModelManager_Create(NULL);
    if (offlineModelBuffer == NULL) {
        LOGE("Failed to create offlineModelBuffer");
        env->ReleaseByteArrayElements(offlineModelBuffer_, offlineModelBuffer,
                                      0);
        return -1;
    }
    int bufferSize = env->GetArrayLength(offlineModelBuffer_);
    modelBuffer = HIAI_MixModelBuffer_Create_From_Buffer(
        modelName, (void *)offlineModelBuffer, bufferSize,
        HIAI_MixDevPerf::HIAI_MIX_DEVPREF_HIGH, isMixModel);
    HIAI_MixModelBuffer *modelBufferArray[] = {modelBuffer};

    int ret = HIAI_MixModel_LoadFromModelBuffers(manager, modelBufferArray, 1);

    LOGI("load model from buffer ret = %d", ret);
    env->ReleaseStringUTFChars(offlineModelName_, modelName);
    env->ReleaseByteArrayElements(offlineModelBuffer_, offlineModelBuffer, 0);
    return ret;
}

extern "C" JNIEXPORT jobjectArray JNICALL
Java_com_example_hiai_ModelManager_runModelSync(
    JNIEnv *env, jclass type, jobject modelInfoObj, jobjectArray inBufferArr) {
    jclass modelInfoCls = env->GetObjectClass(modelInfoObj);
    if (modelInfoCls == NULL) {
        LOGE("can not find modelInfoCls class.");
        return NULL;
    }

    jmethodID getOfflineModelName = env->GetMethodID(
        modelInfoCls, "getOfflineModelName", "()Ljava/lang/String;");
    if (getOfflineModelName == NULL) {
        LOGE("can not find getOfflineModelName method.");
        return NULL;
    }

    jstring jmodelName =
        (jstring)env->CallObjectMethod(modelInfoObj, getOfflineModelName);
    const char *modelName = env->GetStringUTFChars(jmodelName, 0);

    if (NULL == manager || NULL == modelTensorinfo) {
        LOGE("please load model first");
        return NULL;
    }

    if (NULL == inBufferArr) {
        LOGE("please input somedata for the model.");
        return NULL;
    }

    int inputNum = env->GetArrayLength(inBufferArr);

    float *dataBuff[inputNum];
    // init input data
    for (int i = 0; i < inputNum; i++) {
        jobject inputdata = env->GetObjectArrayElement(inBufferArr, i);
        dataBuff[i] = env->GetFloatArrayElements((jfloatArray)inputdata, NULL);
        env->DeleteLocalRef(inputdata);
    }

    HIAI_MixTensorBuffer *inputs[modelTensorinfo->input_cnt];
    HIAI_MixTensorBuffer *outputs[modelTensorinfo->output_cnt];
    int *outputSize = new int[modelTensorinfo->output_cnt];

    for (int i = 0, pos = 0; i < modelTensorinfo->input_cnt; ++i) {
        if (NULL == modelTensorinfo->input_shape) {
            LOGE("failed to get input shape");
            return NULL;
        }
        LOGI("input %d shape show as below : ", i);
        int in_n = modelTensorinfo->input_shape[pos++];
        int in_c = modelTensorinfo->input_shape[pos++];
        int in_h = modelTensorinfo->input_shape[pos++];
        int in_w = modelTensorinfo->input_shape[pos++];

        LOGI("input_n = %d input_c = %d input_h = %d input_w = "
             "%d",
             in_n, in_c, in_h, in_w);
        HIAI_MixTensorBuffer *input =
            HIAI_MixTensorBuffer_Create(in_n, in_c, in_h, in_w); // NCHW
        if (NULL == input) {
            LOGE("fail: HIAI_MixTensorBuffer_Create input");
            HIAI_MixModel_ReleaseModelTensorInfo(modelTensorinfo);
            HIAI_MixModelBuffer_Destroy(modelBuffer);
            HIAI_MixModelManager_Destroy(manager);
            return NULL;
        }

        inputs[i] = input;
    }

    for (int i = 0, pos = 0; i < modelTensorinfo->output_cnt; ++i) {
        if (NULL == modelTensorinfo->output_shape) {
            LOGE("failed to get the output shape");
            return NULL;
        }
        LOGI("output %d shape show as below : ", i);
        int out_n = modelTensorinfo->output_shape[pos++];
        int out_c = modelTensorinfo->output_shape[pos++];
        int out_h = modelTensorinfo->output_shape[pos++];
        int out_w = modelTensorinfo->output_shape[pos++];

        LOGI("output_n = %d output_c = %d output_h = %d "
             "output_w = %d",
             out_n, out_c, out_h, out_w);
        outputSize[i] = out_n * out_c * out_h * out_w;
        HIAI_MixTensorBuffer *output =
            HIAI_MixTensorBuffer_Create(out_n, out_c, out_h, out_w);

        if (NULL == output) {
            LOGE("fail :HIAI_MixTensorBuffer_Create output");
            HIAI_MixModel_ReleaseModelTensorInfo(modelTensorinfo);
            HIAI_MixModelBuffer_Destroy(modelBuffer);
            HIAI_MixModelManager_Destroy(manager);
            return NULL;
        }
        outputs[i] = output;
    }

    for (int i = 0; i < modelTensorinfo->input_cnt; ++i) {
        float *in_data = (float *)HIAI_MixTensorBuffer_GetRawBuffer(inputs[i]);
        int size = HIAI_MixTensorBuffer_GetBufferSize(inputs[i]);
        memcpy(in_data, dataBuff[i], size);
    }

    int ret = HIAI_MixModel_RunModel(
        manager, inputs, modelTensorinfo->input_cnt, outputs,
        modelTensorinfo->output_cnt, 1000, modelName);

    LOGI("run model ret: %d", ret);

    jfloatArray *predictRetArr = new jfloatArray[modelTensorinfo->output_cnt];
    for (int o = 0; o < modelTensorinfo->output_cnt; o++) {
        float *outputBuffer =
            (float *)HIAI_MixTensorBuffer_GetRawBuffer(outputs[o]);
        jfloat *temp = new float[outputSize[o]];
        for (int i = 0; i < outputSize[o]; i++) {
            temp[i] = outputBuffer[i];
        }
        predictRetArr[o] = env->NewFloatArray(outputSize[o]);
        env->SetFloatArrayRegion(predictRetArr[o], 0, outputSize[o], temp);
        delete[] temp;
    }

    if (inputTensors != NULL) {
        HIAI_MixTensorBufferr_Destroy(inputTensors);
        inputTensors = NULL;
    }

    if (outputTensors != NULL) {
        HIAI_MixTensorBufferr_Destroy(outputTensors);
        outputTensors = NULL;
    }

    jclass floatClass = env->FindClass("[F");
    jobjectArray jpredictRetArr =
        env->NewObjectArray(modelTensorinfo->output_cnt, floatClass, NULL);
    for (int i = 0; i < modelTensorinfo->output_cnt; i++) {
        env->SetObjectArrayElement(jpredictRetArr, i, predictRetArr[i]);
    }

    env->ReleaseStringUTFChars(jmodelName, modelName);
    env->DeleteLocalRef(inBufferArr);
    delete[] predictRetArr;
    delete[] outputSize;
    for (int i = 0; i < modelTensorinfo->input_cnt; ++i) {
        if (NULL != inputs[i]) {
            HIAI_MixTensorBufferr_Destroy(inputs[i]);
        }
    }
    for (int i = 0; i < modelTensorinfo->output_cnt; ++i) {
        if (NULL != outputs[i]) {
            HIAI_MixTensorBufferr_Destroy(outputs[i]);
        }
    }

    return jpredictRetArr;
}
