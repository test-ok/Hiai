package com.example.hiai;

import android.content.Context;
import android.content.res.AssetManager;
import android.graphics.Bitmap;
import android.util.Log;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.ByteArrayOutputStream;
import java.io.Closeable;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;

public class Utils {

    private static final String TAG = Utils.class.getSimpleName();
    private static BufferedInputStream bis = null;
    private static InputStream fileInput = null;
    private static FileOutputStream fileOutput = null;
    private static ByteArrayOutputStream byteOut = null;

    public static byte[] getModelBufferFromModelFile(String modelPath) {
        try {
            bis = new BufferedInputStream(new FileInputStream(modelPath));
            byteOut = new ByteArrayOutputStream(1024);
            byte[] buffer = new byte[1024];
            int size = 0;
            while ((size = bis.read(buffer, 0, 1024)) != -1) {
                byteOut.write(buffer, 0, size);
            }
            return byteOut.toByteArray();

        } catch (Exception e) {
            return new byte[0];
        } finally {
            releaseResource(byteOut);
            releaseResource(bis);
        }
    }

    private static void releaseResource(Closeable resource) {

        if (resource != null) {
            try {
                resource.close();
                resource = null;
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
    }

    public static float[] NHWCtoNCHW(float[] orinal, int N, int C, int H, int W) {
        if (orinal == null || orinal.length == 0 || N * H * W * C == 0 || N < 0 || C < 0 || H < 0 || W < 0) {
            return orinal;
        }
        float[] nchw = new float[orinal.length];
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < C; j++) {
                for (int k = 0; k < H * W; k++) {
                    nchw[i * C * H * W + j * H * W + k] = orinal[i * H * W * C + k * C + j];
                }
            }
        }
        return nchw;
    }

    public static float[] NCHWtoNHWC(float[] orinal, int N, int C, int H, int W) {
        if (orinal == null || orinal.length == 0 || N * H * W * C == 0 || N < 0 || C < 0 || H < 0 || W < 0) {
            return orinal;
        }
        float[] nhwc = new float[orinal.length];

        for (int i = 0; i < N; i++) {
            for (int j = 0; j < C; j++) {
                for (int k = 0; k < H * W; k++) {
                    nhwc[i * C * H * W + k * C + j] = orinal[i * C * H * W + j * H * W + k];
                }
            }
        }
        return nhwc;
    }

    public static boolean copyModelsFromAssetToAppModelsByBuffer(AssetManager am, String sourceModelName, String destDir) {

        try {
            fileInput = am.open(sourceModelName);
            String filename = destDir + sourceModelName;

            fileOutput = new FileOutputStream(filename);
            BufferedOutputStream bufferedOutputStream = new BufferedOutputStream(fileOutput);
            byteOut = new ByteArrayOutputStream();
            byte[] buffer = new byte[1024];
            int len = -1;
            while ((len = fileInput.read(buffer)) != -1) {
                bufferedOutputStream.write(buffer, 0, len);
            }
            bufferedOutputStream.close();
            return true;
        } catch (Exception ex) {
            Log.e(TAG, "copyModelsFromAssetToAppModels : " + ex);
            return false;
        } finally {
            releaseResource(byteOut);
            releaseResource(fileOutput);
            releaseResource(fileInput);
        }
    }

    public static boolean copyModelsFromAssetToAppModels(AssetManager am, String sourceModelName, String destDir) {

        try {
            fileInput = am.open(sourceModelName);
            String filename = destDir + sourceModelName;

            fileOutput = new FileOutputStream(filename);
            byteOut = new ByteArrayOutputStream();
            byte[] buffer = new byte[1024];
            int len = -1;
            while ((len = fileInput.read(buffer)) != -1) {
                byteOut.write(buffer, 0, len);
            }
            fileOutput.write(byteOut.toByteArray());
            return true;
        } catch (Exception ex) {
            Log.e(TAG, "copyModelsFromAssetToAppModels : " + ex);
            return false;
        } finally {
            releaseResource(byteOut);
            releaseResource(fileOutput);
            releaseResource(fileInput);
        }
    }

    public static boolean isExistModelsInAppModels(String modelname, String savedir) {

        File dir = new File(savedir);
        File[] currentfiles = dir.listFiles();
        if (currentfiles == null) {
            return false;
        } else {
            for (File file : currentfiles) {
                if (file.getName().equals(modelname)) {
                    return true;
                }
            }
        }
        return false;
    }

    public static boolean copyAssetAndWrite(Context mContext, String fileName) {
        InputStream is = null;
        FileOutputStream fos = null;
        try {
            File cacheDir = mContext.getCacheDir();
            if (!cacheDir.exists()) {
                cacheDir.mkdirs();
            }
            File outFile = new File(cacheDir, fileName);
            if (!outFile.exists()) {
                boolean res = outFile.createNewFile();
                if (!res) {
                    return false;
                }
            } else {
                if (outFile.length() > 10) {
                    return true;
                }
            }
            is = mContext.getAssets().open(fileName);
            fos = new FileOutputStream(outFile);
            byte[] buffer = new byte[1024];
            int byteCount;
            while ((byteCount = is.read(buffer)) != -1) {
                fos.write(buffer, 0, byteCount);
            }
            fos.flush();

            return true;
        } catch (IOException e) {
            e.printStackTrace();
        } finally {
            releaseResource(is);
            releaseResource(fos);
        }

        return false;
    }

    public static float[] getPixelForTensorflow(Bitmap bitmap, int resizedWidth, int resizedHeight) {
        int batch = 1;
        int channel = 3;
        float[] buff = new float[channel * resizedWidth * resizedHeight];
        int [] intValues = new int[resizedWidth * resizedHeight];
        bitmap.getPixels(intValues,0,resizedWidth,0,0,resizedWidth,resizedHeight);
        for (int i = 0;i < intValues.length; i++) {
            final int val = intValues[i];
            buff[i*3+0] = (val & 0xFF0000) >> 16;
            buff[i*3+1] = (val & 0xFF00)   >> 8;
            buff[i*3+2] = (val & 0xFF);
        }
        return NHWCtoNCHW(buff,batch,channel,resizedHeight,resizedWidth);
    }
}
