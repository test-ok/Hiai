package com.example.hiai;

import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.widget.Toast;

import static com.example.hiai.Constant.AI_OK;
public class SyncActivity extends InitActivity {

    private static final String TAG = SyncActivity.class.getSimpleName();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
//        setContentView(R.layout.activity_sync);
    }

    @Override
    protected void loadModelFromFile(EDSRmodelModel e){
        int ret = e.load(am);
        if (AI_OK == ret) {
            Toast.makeText(SyncActivity.this,"load model success.", Toast.LENGTH_SHORT).show();
        } else {
            Toast.makeText(SyncActivity.this,"load model fail.", Toast.LENGTH_SHORT).show();
        }
    }

    @Override
    protected void runModel(EDSRmodelModel e,float[][] inputData) {
        long start = System.currentTimeMillis();
        outputData = e.predict(inputData);
        long end = System.currentTimeMillis();
        inferenceTime = end - start;
        if (outputData == null) {
            Log.e(TAG,"runModelSync fail, outputData is null");
            return;
        }
        Log.i(TAG,"runModel outputdata length: " + outputData.length + "inferenceTime = " + inferenceTime);

        Toast.makeText(SyncActivity.this,"inferenceTime = " + inferenceTime,Toast.LENGTH_SHORT).show();
    }

    @Override
    protected void onResume() {
        super.onResume();
    }
    @Override
    protected void onDestroy() {
        super.onDestroy();
        int result = edsRmodelModel.unload();

        if (AI_OK == result) {
            Toast.makeText(this,"unload model success", Toast.LENGTH_SHORT).show();
        } else {
            Toast.makeText(this,"unload model fail.", Toast.LENGTH_SHORT).show();
        }
    }
}
