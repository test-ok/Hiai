package com.example.hiai;

import java.io.Serializable;

public class ModelInfo implements Serializable {

    private String modelSaveDir = "";
    private String onlineModelLabel = "";
    private String offlineModelName = "";
    private boolean isMixModel = true;

    /**
     * caffe : xxx.prototxt
     * tensorflow : xxx.pb
     * default is "" if don't have online model
     */
    private String onlineModel = "";
    /**
     * caffe: xxx.caffemodel
     * tensorflow: xxx.txt
     * default is "" if don't have online model
     */
    private String onlineModelPara = "";
    /**
     * xxx.cambricon
     * default is "" if don't have offline model
     */
    private String offlineModel = "";

    /**
     * "" or "100.100.001.010" or "100.150.010.010" ...
     * default is "100.100.001.010" if don't know offline model version
     */
    private String offlineModelVersion = "100.100.001.010";

    /**
     * caffe or tensorflow
     */
    private String framework = "";

    public boolean isMixModel() {
        return isMixModel;
    }

    public void setMixModel(boolean mixModel) {
        isMixModel = mixModel;
    }


    public String getModelSaveDir() {
        return modelSaveDir;
    }

    public void setModelSaveDir(String modelSaveDir) {
        this.modelSaveDir = modelSaveDir;
    }

    public String getOnlineModelLabel() {
        return onlineModelLabel;
    }

    public void setOnlineModelLabel(String onlineModelLabel) {
        this.onlineModelLabel = onlineModelLabel;
    }

    public String getOfflineModelName() {
        return offlineModelName;
    }

    public void setOfflineModelName(String offlineModelName) {
        this.offlineModelName = offlineModelName;
    }

    public String getOnlineModel() {
        return onlineModel;
    }

    public void setOnlineModel(String onlineModel) {
        this.onlineModel = onlineModel;
    }

    public String getOnlineModelPara() {
        return onlineModelPara;
    }

    public void setOnlineModelPara(String onlineModelPara) {
        this.onlineModelPara = onlineModelPara;
    }

    public String getOfflineModel() {
        return offlineModel;
    }

    public void setOfflineModel(String offlineModel) {
        this.offlineModel = offlineModel;
    }

    public String getOfflineModelVersion() {
        return offlineModelVersion;
    }

    public void setOfflineModelVersion(String offlineModelVersion) {
        this.offlineModelVersion = offlineModelVersion;
    }

    public String getFramework() {
        return framework;
    }

    public void setFramework(String framework) {
        this.framework = framework;
    }
}
