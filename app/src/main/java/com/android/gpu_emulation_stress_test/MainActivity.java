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

import android.animation.Animator;
import android.animation.AnimatorListenerAdapter;
import android.app.ActivityManager;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ApplicationInfo;
import android.content.pm.ConfigurationInfo;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.res.AssetManager;
import android.graphics.Point;
import android.os.Bundle;
import android.support.design.widget.FloatingActionButton;
import android.support.design.widget.Snackbar;
import android.support.v7.app.AlertDialog;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.Toolbar;
import android.text.TextUtils;
import android.util.Log;
import android.view.Display;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewAnimationUtils;
import android.view.WindowManager;
import android.view.animation.Animation;
import android.view.animation.AnimationUtils;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.RadioGroup;
import android.widget.SeekBar;
import android.widget.TextView;

import java.util.Locale;

public class MainActivity extends AppCompatActivity {

    public static final int REQUEST_CODE = 0;
    public static final int RESULT_CODE_SUCCESS = 1;

    private boolean gles2Ran = false;
    private float gles2Fps = 0.0f;
    private int gles2FpsObjects = 1000;

    private boolean gles3Ran = false;
    private float gles3Fps = 0.0f;
    private int gles3FpsObjects = 1000;

    private boolean supportsGles3 = false;

    private SeekBar mNumObjectsSeekBar;
    private TextView mNumObjectsTextView;



    public static String TAG = MainActivity.class.getSimpleName();
    FloatingActionButton fab;
    RadioGroup radioGroup;

    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        final ActivityManager activityManager =
                (ActivityManager) getSystemService(Context.ACTIVITY_SERVICE);
        final ConfigurationInfo configurationInfo =
                activityManager.getDeviceConfigurationInfo();

        // ro.opengles.version >= 0x30000
        supportsGles3 = configurationInfo.reqGlEsVersion >= 196608;

        setContentView(R.layout.activity_main);


        Toolbar toolbar = (Toolbar) findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);

        radioGroup = (RadioGroup) findViewById(R.id.radioGroup);
        if (supportsGles3) {
            supportsGles3 = true;
            Log.d(TAG, "Device supports OpenGL ES 3.x");
        } else {
            (findViewById(R.id.radioButton_GL3)).setEnabled(false);
            Log.d(TAG, "Device supports OpenGL ES 2.0");
        }

        fab = (FloatingActionButton) findViewById(R.id.fab);
        fab.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                Log.d(TAG, "onClick for play");

                showGraphicsLayout(view);

            }
        });


        mNumObjectsSeekBar = (SeekBar) findViewById(R.id.numObjectsSeekBar);
        mNumObjectsTextView = (TextView) findViewById(R.id.seekbarValue);
        mNumObjectsSeekBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress,
                                          boolean fromUser) {
                // mNumObjectsTextView.setText(Integer.toString(1000 * progress));
                mNumObjectsTextView.setText(String.format(Locale.US, "%,d", (1000 * progress)));
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {
            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {
            }
        });

        if (savedInstanceState != null) {
            if (savedInstanceState.getBoolean("gles3Ran")) {
                setLastFps(
                        savedInstanceState.getFloat("gles3Fps"),
                        3,
                        savedInstanceState.getInt("gles3FpsObjects")
                );
            }

            if (savedInstanceState.getBoolean("gles2Ran")) {
                setLastFps(
                        savedInstanceState.getFloat("gles2Fps"),
                        2,
                        savedInstanceState.getInt("gles2FpsObjects")
                );
            }
        }

        Intent intent = getIntent();
        onActivityResult(0, 1, intent);
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent intent) {

        if (resultCode == RESULT_CODE_SUCCESS) {

            boolean finished =
                    intent.getBooleanExtra("finished", false);
            if (finished) {
                closeGraphicsTouchLayout();
                float fps = intent.getFloatExtra("resultFps", 0.0f);
                int glesApiLevel = intent.getIntExtra("glesVersion", 2);
                int numObjects = intent.getIntExtra("numObjects", 1000);
                setLastFps(fps, glesApiLevel, numObjects);
            }
        }
    }

    @Override
    public void onSaveInstanceState(Bundle savedInstanceState) {
        super.onSaveInstanceState(savedInstanceState);
        savedInstanceState.putBoolean("gles3Ran", gles3Ran);
        savedInstanceState.putBoolean("gles2Ran", gles2Ran);
        savedInstanceState.putFloat("gles3Fps", gles3Fps);
        savedInstanceState.putInt("gles3FpsObjects", gles3FpsObjects);
        savedInstanceState.putFloat("gles2Fps", gles2Fps);
        savedInstanceState.putInt("gles2FpsObjects", gles2FpsObjects);
    }

    public void onPause() {
        super.onPause();
        Log.d(TAG, "onPause!");
    }

    private void runWithApiLevel(int version) {
        int numObjects = 1000 * mNumObjectsSeekBar.getProgress();

        Intent intent = new Intent(this, GPUEmulationStressTestActivity.class);
        intent.putExtra("glesApiLevel", version);
        intent.putExtra("numObjects", numObjects);
        startActivityForResult(intent, REQUEST_CODE);
    }

    public void runGles2(View view) {
        Log.d(TAG, "run GLES 2 test");
        runWithApiLevel(2);
    }

    public void runGles3(View view) {
        Log.d(TAG, "run GLES 3 test");
        runWithApiLevel(3);
    }

    public void setLastFps(float fps, int apiLevel, int numObjects) {
        TextView toSet = null;

        if (apiLevel == 2) {
            gles2Ran = true;
            gles2Fps = fps;
            gles2FpsObjects = numObjects;
            toSet = findViewById(R.id.resultGL2);
        } else if (apiLevel == 3) {
            gles3Ran = true;
            gles3Fps = fps;
            gles3FpsObjects = numObjects;
            toSet = findViewById(R.id.resultGL3);
        }

        if (toSet != null) {
            toSet.setText(this.getString(R.string.test_result_format, numObjects, fps));
        }
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        getMenuInflater().inflate(R.menu.menu_main, menu);
        return true;
    }


    @Override
    public boolean onOptionsItemSelected(MenuItem item) {

        int id = item.getItemId();


        if (id == R.id.action_about) {
            openAboutDialog();
        }

        return super.onOptionsItemSelected(item);
    }

    private void openAboutDialog() {
        String mAppPackageName = this.getPackageName();
        ;

        LayoutInflater inflater = this.getLayoutInflater();
        View view = inflater.inflate(R.layout.version_dialogue, null);
        ImageView logoView = (ImageView) view.findViewById(R.id.app_logo);
        TextView titleView = (TextView) view.findViewById(R.id.app_title);
        TextView versionView = (TextView) view.findViewById(R.id.app_version);

        PackageManager pm = this.getPackageManager();
        try {
            ApplicationInfo applicationInfo = pm.getApplicationInfo(mAppPackageName, 0);
            logoView.setImageDrawable(pm.getApplicationIcon(applicationInfo));

            PackageInfo packageInfo = pm.getPackageInfo(mAppPackageName, 0);
            titleView.setText(pm.getApplicationLabel(applicationInfo));

            String subTitle;
            if (!TextUtils.isEmpty(packageInfo.versionName)
                    && Character.isDigit(packageInfo.versionName.charAt(0))) {
                subTitle = this.getString(
                        R.string.version_number_format, packageInfo.versionName);
            } else {
                subTitle = packageInfo.versionName;
            }
            versionView.setText(subTitle);
        } catch (PackageManager.NameNotFoundException e) {
            Log.d(TAG, "Fetching ApplicationInfo failed.", e);
        }


        AlertDialog.Builder dialog = new AlertDialog.Builder(this);
        dialog.setView(view);
        dialog.create();
        dialog.show();
    }

    private static View sAnimationEndView;

    public void showGraphicsLayout(View view) {

        final View testLayoutView = findViewById(R.id.test_layout);
        final View resultsLayoutView = findViewById(R.id.results_layout);

        int x = fab.getLeft() + (fab.getWidth()) / 2;
        int y = fab.getTop() + (fab.getWidth()) / 2;

        //calculate dimensions of screen size
        Display display = getWindowManager().getDefaultDisplay();
        Point size = new Point();
        display.getSize(size);
        int screenWidth = size.x;
        int screenHeight = size.y;

        int hypotenuse = (int) Math.hypot(screenWidth, screenHeight);

        Animator anim = ViewAnimationUtils.createCircularReveal(testLayoutView, x, y, 0, hypotenuse);
        anim.setDuration(550);

        switch (radioGroup.getCheckedRadioButtonId()) {
            case R.id.radioButton_GL2:
                Snackbar.make(view, "Starting GL2 Test...", Snackbar.LENGTH_LONG)
                        .setAction("Action", null).show();
                break;
            case R.id.radioButton_GL3:
                Snackbar.make(view, "Starting GL3 Test...", Snackbar.LENGTH_LONG)
                        .setAction("Action", null).show();
                break;
        }
        sAnimationEndView = view;

        anim.addListener(new AnimatorListenerAdapter() {
            @Override
            public void onAnimationEnd(Animator animation) {
                super.onAnimationEnd(animation);
                View view = sAnimationEndView;

                toggleFullscreen();
                getSupportActionBar().hide();
                resultsLayoutView.setVisibility(View.INVISIBLE);


                switch (radioGroup.getCheckedRadioButtonId()) {
                    case R.id.radioButton_GL2:
                        runGles2(view);
                        break;
                    case R.id.radioButton_GL3:
                        runGles3(view);
                        break;
                }
            }
        });

        if (testLayoutView == null) {
            Log.d(TAG, "testLayoutView is null!!!!!!!");
        }
        testLayoutView.setVisibility(View.VISIBLE);

        anim.start();

        Animation animation = AnimationUtils.loadAnimation(getApplicationContext(), R.anim.hide);
        fab.startAnimation(animation);
    }

    public void closeGraphicsTouchLayout() {
        final View multiTouchLayoutView = findViewById(R.id.test_layout);
        final View resultsLayoutView = findViewById(R.id.results_layout);

        int x = fab.getLeft() + (fab.getWidth()) / 2;
        int y = fab.getTop() + (fab.getWidth()) / 2;


        Display display = getWindowManager().getDefaultDisplay();
        Point size = new Point();
        display.getSize(size);
        int screenWidth = size.x;
        int screenHeight = size.y;

        int hypotenuse = (int) Math.hypot(screenWidth, screenHeight);

        Animator anim = ViewAnimationUtils.createCircularReveal(multiTouchLayoutView, x, y, hypotenuse, 0);


        toggleFullscreen();
        getSupportActionBar().show();
        resultsLayoutView.setVisibility(View.VISIBLE);


        anim.addListener(new AnimatorListenerAdapter() {
            @Override
            public void onAnimationEnd(Animator animation) {
                super.onAnimationEnd(animation);
                multiTouchLayoutView.setVisibility(View.INVISIBLE);


            }
        });

        anim.start();

        Animation animation = AnimationUtils.loadAnimation(getApplicationContext(), R.anim.show);
        fab.startAnimation(animation);
    }


    public void toggleFullscreen() {
        WindowManager.LayoutParams attrs = getWindow().getAttributes();
        attrs.flags ^= WindowManager.LayoutParams.FLAG_FULLSCREEN;
        getWindow().setAttributes(attrs);
    }
}
