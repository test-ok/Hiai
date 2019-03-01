package com.example.hiai;

import android.content.res.AssetManager;
import android.support.v7.app.AppCompatActivity;

public class EDSRmodelModel extends AppCompatActivity {

    public ModelInfo info = new ModelInfo();

    public EDSRmodelModel(String modelPath, AssetManager mgr){
        initModelInfo(modelPath);

        if(!Utils.isExistModelsInAppModels(info.getOnlineModel(),
        info.getModelSaveDir())){
        Utils.copyModelsFromAssetToAppModelsByBuffer(mgr, info.getOnlineModel(),info.getModelSaveDir());
        } if(!Utils.isExistModelsInAppModels(info.getOnlineModelPara(),info.getModelSaveDir())){
        Utils.copyModelsFromAssetToAppModelsByBuffer(mgr, info.getOnlineModelPara(),info.getModelSaveDir());
        }
        if(!Utils.isExistModelsInAppModels(info.getOfflineModel(),info.getModelSaveDir())){
        Utils.copyModelsFromAssetToAppModelsByBuffer(mgr, info.getOfflineModel(),info.getModelSaveDir());
        }
        modelCompatibilityProcess();
    }

    /**** user load model manager sync interfaces ****/
    public int load(AssetManager mgr){
        return ModelManager.loadModelFromFileSync(info.getOfflineModelName(),
                info.getModelSaveDir()+info.getOfflineModel(),
                info.isMixModel());
    }

    public float[][] predict(float[][] buf){
        return ModelManager.runModelSync(info, buf);
    }

    public int unload(){
        return ModelManager.unloadModelSync();
    }


    /**** load user model async interfaces ****/
    public int registerListenerJNI(ModelManagerListener listener){
        return ModelManager.registerListenerJNI(listener);
    }

    public void loadAsync(AssetManager mgr){
        ModelManager.loadModelFromFileAsync(info.getOfflineModelName(),
            info.getModelSaveDir()+info.getOfflineModel(),
            info.isMixModel());
        return;
    }

    public void predictAsync(float[][] buf) {
        ModelManager.runModelAsync(info, buf);
        return;
    }

    public void unloadAsync(){
        ModelManager.unloadModelAsync();
    }

    private void modelCompatibilityProcess() {
        //load hiai.so
        if (ModelManager.loadJNISo()) {
            ModelManager.modelCompatibilityProcessFromFile(info.getModelSaveDir() + info.getOnlineModel(),
                    info.getModelSaveDir() + info.getOnlineModelPara(),
                    info.getFramework(), info.getModelSaveDir() + info.getOfflineModel(), info.isMixModel());

        }
    }

    private void initModelInfo(String modelPath) {
        info.setModelSaveDir(modelPath);
        info.setOnlineModel("EDSRmodelSegModel.zip");
        info.setFramework("tensorflow");
        info.setOfflineModel("EDSRmodel_offline_SegModel.info");
        info.setOfflineModelName("EDSRmodel_offline_SegModel");
        info.setMixModel(true);
    }
}
