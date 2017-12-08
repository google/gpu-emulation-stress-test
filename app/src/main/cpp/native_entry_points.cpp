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

#include <jni.h>

#include "util.h"

#include "FileLoader.h"
#include "OBJParse.h"
#include "TextureLoader.h"
#include "WorldState.h"
#include "GLES2Renderer.h"
#include "GLES3Renderer.h"

#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>

WorldState *sWorld = nullptr;
GLES2Renderer *sRenderer = nullptr;

JNIEnv *gEnv = nullptr;
jobject glview;

extern "C"
JNIEXPORT void JNICALL
Java_com_android_gpu_1emulation_1stress_1test_GPUEmulationStressTestView_registerGLView(JNIEnv *env, jobject view) {
    gEnv = env;
    glview = view;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_android_gpu_1emulation_1stress_1test_GPUEmulationStressTestView_initAssets(
        JNIEnv *env,
        jobject /* this */,
        jobject java_assetManager, jint glesApiLevel, jint numObjects) {
    FileLoader *fl = FileLoader::get();
    AAssetManager *mgr = AAssetManager_fromJava(env, java_assetManager);
    assert(mgr);
    fl->initWithAssetManager(mgr);

    TextureLoader *tl = TextureLoader::get();

    sWorld = new WorldState;
    sWorld->loadFromFile("gpu_stress_test.esys", numObjects);

    if (glesApiLevel == 2) {
        sRenderer = new GLES2Renderer;
    } else {
        sRenderer = new GLES3Renderer;
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_android_gpu_1emulation_1stress_1test_GPUEmulationStressTestView_reinitGL(
        JNIEnv *env,
        jobject /* this */,
        jint width, jint height) {

    sWorld->resetAspectRatio(width, height);
    sRenderer->reInit(sWorld, width, height);

}

static void sFinishWithFps(float fps) {
    jclass glviewclass = gEnv->FindClass("com/android/gpu_emulation_stress_test/GPUEmulationStressTestView");
    jmethodID method = gEnv->GetStaticMethodID(glviewclass, "finishTest", "(F)V");
    if (!method) {
        LOGD("Function finishBenchmark() not found.");
        return;
    }
    gEnv->CallStaticVoidMethod(glviewclass, method, fps);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_android_gpu_1emulation_1stress_1test_GPUEmulationStressTestView_drawFrame(
        JNIEnv *env,
        jobject /* this */,
        jint) {
    gEnv = env;

    if (sWorld->update()) {
        sRenderer->preDrawUpdate();
        sRenderer->draw();
    } else if (sWorld->done) {
        sFinishWithFps(sWorld->fps);
    }

    return;
}
