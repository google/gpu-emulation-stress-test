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

#include "TextureLoader.h"

#include "lodepng.h"
#include "log.h"
#include "FileLoader.h"

static TextureLoader *sTextureLoader = nullptr;

TextureLoader::TextureLoader() {}

// static
TextureLoader *TextureLoader::get() {
    if (!sTextureLoader)
        sTextureLoader = new TextureLoader;
    return sTextureLoader;
}

std::vector<unsigned char> TextureLoader::loadPNGAsRGBA8(const std::string &filename,
                                                         unsigned int &w,
                                                         unsigned int &h) {
    std::vector<unsigned char> pngData =
            FileLoader::get()->loadFileFromAssets(filename);
    std::vector<unsigned char> res;
    lodepng::decode(res, w, h, pngData);
    return res;
}
