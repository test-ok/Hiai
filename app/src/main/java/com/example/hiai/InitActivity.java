package com.example.hiai;

import android.Manifest;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.res.AssetManager;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.net.Uri;
import android.provider.MediaStore;
import android.support.annotation.NonNull;
import android.support.v4.app.ActivityCompat;
import android.support.v4.content.ContextCompat;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.widget.TextView;
import android.widget.Toast;

import java.io.File;
import java.io.IOException;

import static com.example.hiai.Constant.GALLERY_REQUEST_CODE;
import static com.example.hiai.Constant.IMAGE_CAPTURE_REQUEST_CODE;

public abstract class InitActivity extends AppCompatActivity {
    private static final String TAG = InitActivity.class.getSimpleName();
    protected AssetManager mgr;
    protected Bitmap InputImg;
    protected Bitmap OutputSuImg;

    protected float inferenceTime;
    protected float[][] outputData;
    protected float[][] inputData;
    File dir;
    String path;
    AssetManager am;
    EDSRmodelModel edsRmodelModel;


    // Used to load the 'native-lib' library on application startup.
//    static {
//        System.loadLibrary("native-lib");
//    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_init);

        mgr = getResources().getAssets();
        dir = getDir("models",Context.MODE_PRIVATE);
        path = dir.getAbsolutePath() + File.separator;
        am = getResources().getAssets();

        edsRmodelModel = new EDSRmodelModel(path,am);

        Log.i("QG","START loadModelFromFile" + edsRmodelModel.info.getModelSaveDir() + edsRmodelModel.info.getOfflineModel());

        loadModelFromFile(edsRmodelModel);
        checkStoragePermission();


        // Example of a call to a native method
//        TextView tv = (TextView) findViewById(R.id.sample_text);
//        tv.setText(stringFromJNI());
    }

    private void checkStoragePermission() {
        if (ContextCompat.checkSelfPermission(this,Manifest.permission.WRITE_EXTERNAL_STORAGE) !=
                PackageManager.PERMISSION_GRANTED &&
                ContextCompat.checkSelfPermission(this,Manifest.permission.CAMERA) != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(this,new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE, Manifest.permission.CAMERA},
                    GALLERY_REQUEST_CODE);
            chooseImageAndSuperSo();
        } else {
            chooseImageAndSuperSo();
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (requestCode == GALLERY_REQUEST_CODE && grantResults[1] == PackageManager.PERMISSION_GRANTED) {
            if (grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                Toast.makeText(InitActivity.this,"Permission Get", Toast.LENGTH_SHORT).show();
            } else {
                Toast.makeText(InitActivity.this,"Permission Denied", Toast.LENGTH_SHORT).show();
            }
        }
        if (requestCode == IMAGE_CAPTURE_REQUEST_CODE) {
            if (grantResults[0] == PackageManager.PERMISSION_GRANTED && grantResults[1] == PackageManager.PERMISSION_GRANTED) {
                Toast.makeText(InitActivity.this,"Permission Get", Toast.LENGTH_SHORT).show();
            } else {
                Toast.makeText(InitActivity.this,"Permission Denied", Toast.LENGTH_SHORT).show();
            }
        }
    }

    private void chooseImageAndSuperSo() {
        Intent intent = new Intent(Intent.ACTION_PICK, null);
        intent.setDataAndType(MediaStore.Images.Media.EXTERNAL_CONTENT_URI,"image/*");
        startActivityForResult(intent,GALLERY_REQUEST_CODE);
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode,resultCode,data);
        if (resultCode == RESULT_OK && data != null) {
            try {
                ContentResolver resolver = getContentResolver();
                Uri originalUri = data.getData();
                InputImg = MediaStore.Images.Media.getBitmap(resolver,originalUri);
                String[] proj = {MediaStore.Images.Media.DATA};
                Cursor cursor = managedQuery(originalUri, proj, null,null,null);
                cursor.moveToFirst();
                Log.i("QG","IMAGE_CAPTURE_REQUEST_CODE getInput_W:" + "50" + ",getInput_H:" + "50");
                float[] inputdata = Utils.getPixelForTensorflow(InputImg,50,50);
                inputData = new float[1][];
                inputData[0] = inputdata;
                runModel(edsRmodelModel,inputData);
                Toast.makeText(InitActivity.this,"OK!",Toast.LENGTH_SHORT).show();
            } catch (IOException e) {
                Log.e(TAG,e.toString());
            }
        } else {
            Toast.makeText(InitActivity.this,"Return without pictures", Toast.LENGTH_SHORT).show();
        }
    }
    protected abstract void runModel(EDSRmodelModel e, float[][] inputData);
    protected abstract void loadModelFromFile(EDSRmodelModel e);

    @Override
    protected void onResume() {
        super.onResume();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
    }
    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
    public native String stringFromJNI();
}
