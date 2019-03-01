#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <android/log.h>
#include <cmath>
#include <cstring>
#include <jni.h>
#include <memory.h>
#include <stdlib.h>
#include <string>
#include <sys/time.h>
#include <unistd.h>
#include <vector>

#include "HIAIMixModel.h"

#define LOG_TAG "ASYNC_DDK_MSG"

#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

static HIAI_MixModelManager *modelManager = NULL;

static HIAI_MixModelBuffer *modelBuffer = NULL;

static HIAI_MixModelTensorInfo *modelTensorinfo = NULL;

static std::string loadedModelName = "";
std::vector<HIAI_MixTensorBuffer *> outputTensor;

static HIAI_MixModelListener listener;
static jclass callbacksClass;
static jobject callbacksInstance;
JavaVM *jvm;

int *asyncOutputSize = NULL;
float time_use;
struct timeval tpstart, tpend;

void onLoadDone(void *userdata, int taskId) {

    LOGI("AYSNC JNI layer onLoadDone:%d", taskId);

    JNIEnv *env;
    // load model successfully.
    if (taskId > 0) {
        modelTensorinfo = HIAI_MixModel_GetModelTensorInfo(
            modelManager, loadedModelName.c_str());
    }
    jvm->AttachCurrentThread(&env, NULL);

    if (callbacksInstance != NULL) {
        jmethodID onValueReceived =
            env->GetMethodID(callbacksClass, "onStartDone", "(I)V");
        env->CallVoidMethod(callbacksInstance, onValueReceived, taskId);
    }
}

void onRunDone(void *userdata, int taskStamp) {
    gettimeofday(&tpend, NULL);
    time_use = 1000000 * (tpend.tv_sec - tpstart.tv_sec) + tpend.tv_usec -
               tpstart.tv_usec;

    LOGI("AYSNC infrence time %f ms.", time_use / 1000);
    LOGI("AYSNC JNI layer onRunDone taskStamp: %d", taskStamp);

    JNIEnv *env;
    jvm->AttachCurrentThread(&env, NULL);

    jfloatArray *predictRetArr = new jfloatArray[modelTensorinfo->output_cnt];

    for (int o = 0; o < modelTensorinfo->output_cnt; o++) {
        float *outputBuffer =
            (float *)HIAI_MixTensorBuffer_GetRawBuffer(outputTensor[o]);
        float *temp = new float[asyncOutputSize[o]];
        for (int i = 0; i < asyncOutputSize[o]; i++) {
            temp[i] = outputBuffer[i];
        }

        predictRetArr[o] = env->NewFloatArray(asyncOutputSize[o]);
        env->SetFloatArrayRegion(predictRetArr[o], 0, asyncOutputSize[o], temp);
        delete[] temp;
    }

    jfloat infertime = time_use;

    if (callbacksInstance != NULL) {

        jmethodID onValueReceived =
            env->GetMethodID(callbacksClass, "onRunDone", "(I[[FF)V");
        jclass floatClass = env->FindClass("[F");
        jobjectArray jpredictRetArr =
            env->NewObjectArray(modelTensorinfo->output_cnt, floatClass, NULL);

        for (int i = 0; i < modelTensorinfo->output_cnt; i++) {
            env->SetObjectArrayElement(jpredictRetArr, i, predictRetArr[i]);
        }

        env->CallVoidMethod(callbacksInstance, onValueReceived, taskStamp,
                            jpredictRetArr, infertime);
    }

    delete[] predictRetArr;
    delete[] asyncOutputSize;
    for (int i = 0; i < outputTensor.size(); i++) {
        if (NULL != outputTensor[i]) {
            HIAI_MixTensorBufferr_Destroy(outputTensor[i]);
        }
    }
    outputTensor.clear();
}

void onUnloadDone(void *userdata, int taskStamp) {
    LOGI("JNI layer onUnloadDone: %d", taskStamp);

    JNIEnv *env;

    jvm->AttachCurrentThread(&env, NULL);

    if (callbacksInstance != NULL) {
        jmethodID onValueReceived =
            env->GetMethodID(callbacksClass, "onStopDone", "(I)V");
        env->CallVoidMethod(callbacksInstance, onValueReceived, taskStamp);
    }

    if (NULL != modelTensorinfo) {
        HIAI_MixModel_ReleaseModelTensorInfo(modelTensorinfo);
    }

    HIAI_MixModelManager_Destroy(modelManager);

    modelManager = NULL;

    listener.onRunDone = NULL;
    listener.onUnloadDone = NULL;
    listener.onTimeout = NULL;
    listener.onError = NULL;
    listener.onLoadDone = NULL;
}

void onTimeout(void *userdata, int taskStamp) {
    LOGI("JNI layer onTimeout: %d", taskStamp);

    JNIEnv *env;

    jvm->AttachCurrentThread(&env, NULL);

    if (callbacksInstance != NULL) {
        jmethodID onValueReceived =
            env->GetMethodID(callbacksClass, "onTimeout", "(I)V");
        env->CallVoidMethod(callbacksInstance, onValueReceived, taskStamp);
    }
}

void onError(void *userdata, int taskStamp, int errCode) {
    LOGI("JNI layer onError: %d", taskStamp);

    JNIEnv *env;

    jvm->AttachCurrentThread(&env, NULL);

    if (callbacksInstance != NULL) {
        jmethodID onValueReceived =
            env->GetMethodID(callbacksClass, "onError", "(II)V");
        env->CallVoidMethod(callbacksInstance, onValueReceived, taskStamp,
                            errCode);
    }
}

extern "C" JNIEXPORT jint JNICALL
Java_com_example_hiai_ModelManager_registerListenerJNI(
    JNIEnv *env, jclass obj, jobject callbacks) {
    env->GetJavaVM(&jvm);
    callbacksInstance = env->NewGlobalRef(callbacks);
    jclass objClass = env->GetObjectClass(callbacks);
    if (objClass) {
        callbacksClass = reinterpret_cast<jclass>(env->NewGlobalRef(objClass));
        env->DeleteLocalRef(objClass);
    }

    listener.onLoadDone = onLoadDone;
    listener.onRunDone = onRunDone;
    listener.onUnloadDone = onUnloadDone;
    listener.onTimeout = onTimeout;
    listener.onError = onError;

    modelManager = HIAI_MixModelManager_Create(&listener);
    return 0;
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_hiai_ModelManager_loadModelAsync(
    JNIEnv *env, jclass instance, jstring jmodelName, jstring jofflinemodelfile,
    jobject assetManager, jboolean isMixModel) {
    const char *modelName = env->GetStringUTFChars(jmodelName, 0);
    const char *offlinemodelfile = env->GetStringUTFChars(jofflinemodelfile, 0);

    AAssetManager *mgr = AAssetManager_fromJava(env, assetManager);
    LOGI("Attempting to load model...\n");

    LOGI("model name is %s", modelName);

    AAsset *asset =
        AAssetManager_open(mgr, offlinemodelfile, AASSET_MODE_BUFFER);

    if (nullptr == asset) {
        LOGE("AAsset is null\n");
        return;
    }

    const void *data = AAsset_getBuffer(asset);

    if (nullptr == data) {
        LOGE("model buffer is null\n");
        return;
    }

    off_t len = AAsset_getLength(asset);

    if (0 == len) {
        LOGE("model buffer length is 0\n");
        return;
    }

    loadedModelName = std::string(modelName);
    modelBuffer = HIAI_MixModelBuffer_Create_From_Buffer(
        modelName, (void *)data, len, HIAI_MIX_DEVPREF_HIGH, isMixModel);
    HIAI_MixModelBuffer *modelBufferArr[] = {modelBuffer};

    int ret =
        HIAI_MixModel_LoadFromModelBuffers(modelManager, modelBufferArr, 1);

    LOGI("ASYNC JNI LAYER load model from assets ret = %d", ret);
    env->ReleaseStringUTFChars(jmodelName, modelName);
    env->ReleaseStringUTFChars(jofflinemodelfile, offlinemodelfile);

    AAsset_close(asset);
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_hiai_ModelManager_unloadModelAsync(
    JNIEnv *env, jclass instance) {
    if (NULL == modelManager) {
        LOGE("please load model first");
        return;
    }
    if (modelBuffer != NULL) {
        HIAI_MixModelBuffer_Destroy(modelBuffer);
        modelBuffer = NULL;
    }

    if (modelTensorinfo != NULL) {
        HIAI_MixModel_ReleaseModelTensorInfo(modelTensorinfo);
        modelTensorinfo = NULL;
    }

    int ret = HIAI_MixModel_UnLoadModel(modelManager);
    LOGI("ASYNC JNI layer unLoadModel ret:%d", ret);
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_hiai_ModelManager_loadModelFromFileAsync(
    JNIEnv *env, jclass type, jstring modelName_, jstring modelpath_,
    jboolean isMixModel) {
    const char *modelName = env->GetStringUTFChars(modelName_, 0);
    const char *modelPath = env->GetStringUTFChars(modelpath_, 0);

    LOGI("modelPath is %s", modelPath);
    loadedModelName = std::string(modelName);
    modelBuffer = HIAI_MixModelBuffer_Create_From_File(
        modelName, modelPath, HIAI_MIX_DEVPREF_HIGH, isMixModel);
    HIAI_MixModelBuffer *modelBufferArr[] = {modelBuffer};

    if (modelBuffer == NULL) {
        LOGE("modelBuffer1 is NULL");
        env->ReleaseStringUTFChars(modelName_, modelName);
        env->ReleaseStringUTFChars(modelpath_, modelPath);
        return;
    }
    int ret =
        HIAI_MixModel_LoadFromModelBuffers(modelManager, modelBufferArr, 1);

    LOGI("Async load model from file ret = %d", ret);

    env->ReleaseStringUTFChars(modelName_, modelName);
    env->ReleaseStringUTFChars(modelpath_, modelPath);
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_hiai_ModelManager_loadModelAsyncFromBuffer(
    JNIEnv *env, jclass type, jstring offlineModelName_,
    jbyteArray jofflineModelBufferArr, jboolean isMixModel) {
    const char *offlineModelName = env->GetStringUTFChars(offlineModelName_, 0);
    jbyte *offlineModelBuffer =
        env->GetByteArrayElements(jofflineModelBufferArr, NULL);

    if (offlineModelBuffer == NULL) {
        LOGE("offlineModelBuffer is NULL");
        env->ReleaseStringUTFChars(offlineModelName_, offlineModelName);
        env->ReleaseByteArrayElements(jofflineModelBufferArr,
                                      offlineModelBuffer, 0);
        return;
    }

    int bufferSize = env->GetArrayLength(jofflineModelBufferArr);
    loadedModelName = std::string(offlineModelName);
    HIAI_MixModelBuffer *modelBuffer = HIAI_MixModelBuffer_Create_From_Buffer(
        offlineModelName, (void *)offlineModelBuffer, bufferSize,
        HIAI_MIX_DEVPREF_HIGH, isMixModel);
    HIAI_MixModelBuffer *modelBufferArr[] = {modelBuffer};

    int ret =
        HIAI_MixModel_LoadFromModelBuffers(modelManager, modelBufferArr, 1);

    LOGI("load model from buffer ret = %d", ret);

    env->ReleaseStringUTFChars(offlineModelName_, offlineModelName);
    env->ReleaseByteArrayElements(jofflineModelBufferArr, offlineModelBuffer,
                                  0);
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_hiai_ModelManager_runModelAsync(
    JNIEnv *env, jclass type, jobject jmodelInfo, jobjectArray inBufferArr) {

    if (NULL == modelManager || NULL == modelTensorinfo) {
        LOGE("please load model first");
        return;
    }

    jclass jmodelInfoCls = env->GetObjectClass(jmodelInfo);
    if (jmodelInfoCls == NULL) {
        LOGE("can not find jmodelInfoCls class.");
        return;
    }

    jmethodID getOfflineModelName = env->GetMethodID(
        jmodelInfoCls, "getOfflineModelName", "()Ljava/lang/String;");
    if (getOfflineModelName == NULL) {
        LOGE("can not find getOfflineModelName method.");
        return;
    }

    jstring jmodelName =
        (jstring)env->CallObjectMethod(jmodelInfo, getOfflineModelName);
    const char *modelName = env->GetStringUTFChars(jmodelName, 0);

    if (NULL == inBufferArr) {
        LOGE("the input buffer can't be NULL.");
        return;
    }

    int inputNum = env->GetArrayLength(inBufferArr);
    LOGI("inputNum is %d ", inputNum);
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
            return;
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
            HIAI_MixModelManager_Destroy(modelManager);
            return;
        }

        inputs[i] = input;
    }

    for (int i = 0, pos = 0; i < modelTensorinfo->output_cnt; ++i) {
        if (NULL == modelTensorinfo->output_shape) {
            LOGE("failed to get the output shape");
            return;
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
            HIAI_MixModelManager_Destroy(modelManager);
            return;
        }
        outputs[i] = output;
    }

    asyncOutputSize = outputSize;
    // init  input
    for (int i = 0; i < modelTensorinfo->input_cnt; ++i) {
        float *in_data = (float *)HIAI_MixTensorBuffer_GetRawBuffer(inputs[i]);
        int size = HIAI_MixTensorBuffer_GetBufferSize(inputs[i]);
        memcpy(in_data, dataBuff[i], size);
    }

    gettimeofday(&tpstart, NULL);

    int ret = HIAI_MixModel_RunModel(
        modelManager, inputs, modelTensorinfo->input_cnt, outputs,
        modelTensorinfo->output_cnt, 1000, modelName);

    LOGI("ASYNC JNI layer runmodel ret: %d", ret);

    // listener.userdata = NULL;
    // listener.userdata = outputs;
    for (int i = 0; i < modelTensorinfo->input_cnt; i++) {
        if (NULL != inputs[i]) {
            HIAI_MixTensorBufferr_Destroy(inputs[i]);
        }
    }

    env->ReleaseStringUTFChars(jmodelName, modelName);
    env->DeleteLocalRef(inBufferArr);
}