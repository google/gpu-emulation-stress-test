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
import android.content.Intent;
import android.content.res.AssetManager;
import android.os.Bundle;
import android.view.View;

public class GPUEmulationStressTestActivity extends Activity {
    public static String TAG = "GPUEmulationStressTestActivity";
    private GPUEmulationStressTestView mGPUEmulationStressTestView;
    protected AssetManager mAssetManager;

    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        mAssetManager = getAssets();
        Intent intent = getIntent();
        int version = intent.getIntExtra("glesApiLevel", 2);
        int numObjects = intent.getIntExtra("numObjects", 1000);

        getWindow().getDecorView().setSystemUiVisibility(
                getWindow().getDecorView().getSystemUiVisibility() |
                        View.SYSTEM_UI_FLAG_HIDE_NAVIGATION |
                        View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY);

        mGPUEmulationStressTestView = new GPUEmulationStressTestView(this, mAssetManager, version, numObjects);
        setContentView(mGPUEmulationStressTestView);
    }
}
