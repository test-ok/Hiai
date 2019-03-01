package com.example.hiai;

import android.os.Bundle;
import android.util.Log;
import android.widget.Toast;

import static com.example.hiai.Constant.AI_OK;
public class AsyncActivity extends InitActivity {

    private static final String TAG = AsyncActivity.class.getSimpleName();
    ModelManagerListener listener = new ModelManagerListener() {
        @Override
        public void onStartDone(final int taskId) {
            Log.i(TAG,"java layer onStartDone:" + taskId);

            runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    if (taskId > 0) {
                        Toast.makeText(AsyncActivity.this,"load model success. taskId is:" + taskId, Toast.LENGTH_SHORT).show();
                    } else {
                        Toast.makeText(AsyncActivity.this,"load model fail. taskId is: " + taskId, Toast.LENGTH_SHORT).show();
                    }
                }
            });
        }

        @Override
        public void onRunDone(final int taskId, final float[] output, final float inferencetime) {
            Log.i(TAG,"java layer onRunDone:" + taskId);
            runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    if (taskId > 0) {
                        Toast toast = Toast.makeText(AsyncActivity.this,"run model success. taskId is: " + taskId, Toast.LENGTH_SHORT );
                        toast.show();
                        outputData[0] = output;
                        inferenceTime = inferencetime / 1000;
                        Log.i(TAG,"inferenceTime = " + inferenceTime);
                        Toast.makeText(AsyncActivity.this,"inferenceTime = " + inferenceTime,Toast.LENGTH_SHORT).show();
                    } else {
                        Toast toast = Toast.makeText(AsyncActivity.this,"run model fail. taskId is: " + taskId, Toast.LENGTH_SHORT);
                        toast.show();
                    }
                }
            });
        }

        @Override
        public void onStopDone(final int taskId) {
            Log.e(TAG,"java layer onStopDone: " + taskId);

            runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    if (taskId > 0) {
                        Toast.makeText(AsyncActivity.this,"unload model success. taskId is :" + taskId, Toast.LENGTH_SHORT).show();
                    } else {
                        Toast.makeText(AsyncActivity.this,"unload model fail. taskId is :" + taskId, Toast.LENGTH_SHORT).show();
                    }
                }
            });
        }

        @Override
        public void onTimeout(int taskId) {
            Log.e(TAG,"java layer onTimeout: " + taskId);
        }

        @Override
        public void onError(int taskId, int errCode) {
            Log.e(TAG,"onError: " + taskId + "errorCode:" + errCode);
        }

        @Override
        public void onServiceDied() {
            Log.e(TAG,"onServiceDied: ");
        }
    };

//    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
//        setContentView(R.layout.activity_async);
    }
    @Override
    protected void loadModelFromFile(EDSRmodelModel e) {
        int ret = e.registerListenerJNI(listener);
        Log.i(TAG,"loadModelFromFile: " + ret);
        if (AI_OK == ret) {
            Toast.makeText(this,"load model success.", Toast.LENGTH_SHORT).show();
        } else {
            Toast.makeText(this,"load model fail", Toast.LENGTH_SHORT).show();
        }
        e.loadAsync(am);
    }

    @Override
    protected void runModel(EDSRmodelModel e, float[][] inputData) {
        Log.i(TAG,"runModel");
        e.predictAsync(inputData);
    }

    @Override
    protected void onResume() {
        super.onResume();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        edsRmodelModel.unloadAsync();
    }
}
