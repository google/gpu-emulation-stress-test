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

#pragma once

#include <android/log.h>

#define  LOG_TAG    "GLTestViewJNI"
#define  LOGD(fmt, ...)  __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,"%s:%d: " fmt "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define  LOGV(fmt, ...)  __android_log_print(ANDROID_LOG_VERBOSE,LOG_TAG,"%s:%d: " fmt "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)