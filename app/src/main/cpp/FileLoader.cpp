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

#include "FileLoader.h"

#include "log.h"

#include <stdlib.h>
#include <string.h>

static FileLoader *sFileLoader = nullptr;

// static
FileLoader *FileLoader::get() {
    if (!sFileLoader) sFileLoader = new FileLoader();
    return sFileLoader;
}

void FileLoader::initWithAssetManager(AAssetManager *assetManager) {
    mAssetManager = assetManager;
}

std::vector<unsigned char> FileLoader::loadFileFromAssets(const std::string &filename) {
    LOGV("%s: file %s", __func__, filename.c_str());

    std::vector<unsigned char> empty;

    AAsset *asset = AAssetManager_open(
            mAssetManager, filename.c_str(),
            AASSET_MODE_UNKNOWN);
    if (asset) {
        int assetLength = AAsset_getLength(asset);
        LOGV("file: %s bytes: %d", filename.c_str(), assetLength);

        if (assetLength <= 0) return empty;

        buffer = (char *) malloc((unsigned int) assetLength + 1);
        AAsset_read(asset, buffer, (unsigned int) assetLength);
        buffer[assetLength] = 0;

        std::vector<unsigned char> res(assetLength + 1);
        memcpy(&res[0], buffer, assetLength + 1);

        AAsset_close(asset);
        free(buffer);
        return res;
    }

    LOGE("Error reading file %s", filename.c_str());

    return empty;
}
