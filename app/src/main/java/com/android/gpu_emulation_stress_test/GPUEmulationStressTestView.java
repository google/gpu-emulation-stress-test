/*
* Copyright (C) 2017 The Android Open Source Project
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

package com.android.gpu_emulation_stress_test;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.res.AssetManager;
import android.opengl.GLSurfaceView;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.util.Log;
import android.view.View;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

public class GPUEmulationStressTestView extends GLSurfaceView {

    static {
        System.loadLibrary("native_entry_points");
    }

    /* Entry points to native libraries for rendering. */
    public static native void initAssets(AssetManager mgr, int glesApiLevel, int numObjects);

    public static native void reinitGL(int width, int height);

    public static native void drawFrame();

    public static native void registerGLView(GPUEmulationStressTestView view);

    private static String TAG = "GLTestView";
    private static final boolean DEBUG = false;

    protected AssetManager mAssetManager;

    private static GPUEmulationStressTestView currGLView;
    private static Thread currThread;
    private static Handler currHandler;
    private static int mGlesVersion;
    private static int mNumObjects;
    private static Intent mIntent;

    public GPUEmulationStressTestView(Context context, AssetManager assets,
                                      int glesVersion, int numObjects) {
        super(context);

        currGLView = this;
        registerGLView(this);

        mNumObjects = numObjects;
        mGlesVersion = glesVersion;
        mAssetManager = assets;

        // Initialize assets and the world based on
        // GLES version and number of objects.
        initAssets(mAssetManager, mGlesVersion, mNumObjects);

        // Create an OpenGL ES 2 or 3 context based on
        // the input.
        setEGLContextClientVersion(mGlesVersion);
        setRenderer(new GPUStressTestRenderer());

        // This handler is meant purely to catch the signal for when
        // the benchmark ends.
        currThread = Thread.currentThread();
        currHandler = new Handler(Looper.getMainLooper()) {
            @Override
            public void handleMessage(Message msg) {
                super.handleMessage(msg);
                if (msg.arg1 == 1) {
                    Log.d(TAG, "got quit message");
                    currGLView.setVisibility(View.GONE);

                    Context context = getContext();
                    Intent intent = new Intent(context, MainActivity.class);
                    intent.putExtra("finished", true);
                    intent.putExtra("resultFps", ((GPUEmulationStats) msg.obj).fps());
                    intent.putExtra("numObjects", mNumObjects);
                    intent.putExtra("glesVersion", mGlesVersion);

                    Activity act = (Activity) context;
                    act.setResult(MainActivity.RESULT_CODE_SUCCESS, intent);
                    act.finish();
                }
            }
        };
    }

    // Called from native code. Needs to synchronize with class names
    // and function signatures in native_entry_points.cpp.
    public static void finishTest(float fpsAvg) {
        Log.d(TAG, "Finish with " + fpsAvg + " fps");

        Message fpsmsg = Message.obtain();
        fpsmsg.arg1 = 1;
        fpsmsg.obj = new GPUEmulationStats(fpsAvg);
        currHandler.sendMessage(fpsmsg);
    }

    private static class GPUStressTestRenderer implements GLSurfaceView.Renderer {
        public void onDrawFrame(GL10 gl) {
            drawFrame();
        }

        public void onSurfaceChanged(GL10 gl, int width, int height) {
            reinitGL(width, height);
        }

        public void onSurfaceCreated(GL10 gl, EGLConfig config) {
            // Do nothing.
        }
    }

}
