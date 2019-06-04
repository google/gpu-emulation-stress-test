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

// Based on https://github.com/LFY/lel-engine/blob/master/lel/OBJData.cpp

#include "OBJParse.h"

#include "FileLoader.h"

#include "util.h"

OBJParse::OBJParse(const std::string &objFileName) {

    std::vector<unsigned char> objContents =
            FileLoader::get()->loadFileFromAssets(objFileName);

    std::string objStr(objContents.begin(), objContents.end());

    std::vector<std::string> objLines = splitLines(objStr);

    float x, y, z;
    unsigned int p0, t0, n0, p1, t1, n1, p2, t2, n2;

    for (const auto &line : objLines) {
        if (sscanf(&line[0], "v %f %f %f", &x, &y, &z) == 3) {
            obj_v.push_back({x, y, z});
        } else if (sscanf(&line[0], "vn %f %f %f", &x, &y, &z) == 3) {
            obj_vn.push_back({x, y, z});
        } else if (sscanf(&line[0], "vt %f %f", &x, &y) == 2) {
            obj_vt.push_back({x, 1.0f - y}); // OpenGL has flipped texcoords vs Blender
        } else if (sscanf(&line[0], "f %u/%u/%u %u/%u/%u %u/%u/%u",
                          &p0, &t0, &n0, &p1, &t1, &n1, &p2, &t2, &n2) == 9) {
            obj_f.push_back({p0, t0, n0,
                             p1, t1, n1,
                             p2, t2, n2});
        }
    }

    for (unsigned int i = 0; i < (unsigned int) obj_f.size(); i++) {
        unsigned int vertA = obj_f[i][0] - 1;
        unsigned int vertB = obj_f[i][3] - 1;
        unsigned int vertC = obj_f[i][6] - 1;

        unsigned int normA = obj_f[i][2] - 1;
        unsigned int normB = obj_f[i][5] - 1;
        unsigned int normC = obj_f[i][8] - 1;

        unsigned int texA = obj_f[i][1] - 1;
        unsigned int texB = obj_f[i][4] - 1;
        unsigned int texC = obj_f[i][7] - 1;

        VertexKey keyA = {vertA, normA, texA};
        VertexKey keyB = {vertB, normB, texB};
        VertexKey keyC = {vertC, normC, texC};

        VertexAttributes &currDataA = vertexDataMap[keyA];
        VertexAttributes &currDataB = vertexDataMap[keyB];
        VertexAttributes &currDataC = vertexDataMap[keyC];

        indexDataMap[keyA] = 0;
        indexDataMap[keyB] = 0;
        indexDataMap[keyC] = 0;

        unsigned int posAttribBytes = sizeof(float) * 3;
        unsigned int normAttribBytes = sizeof(float) * 3;
        unsigned int texcoordAttribBytes = sizeof(float) * 2;

        memcpy(&currDataA.pos, &obj_v[vertA][0], posAttribBytes);
        memcpy(&currDataB.pos, &obj_v[vertB][0], posAttribBytes);
        memcpy(&currDataC.pos, &obj_v[vertC][0], posAttribBytes);

        memcpy(&currDataA.norm, &obj_vn[normA][0], normAttribBytes);
        memcpy(&currDataB.norm, &obj_vn[normB][0], normAttribBytes);
        memcpy(&currDataC.norm, &obj_vn[normC][0], normAttribBytes);

        memcpy(&currDataA.texcoord, &obj_vt[texA][0], texcoordAttribBytes);
        memcpy(&currDataB.texcoord, &obj_vt[texB][0], texcoordAttribBytes);
        memcpy(&currDataC.texcoord, &obj_vt[texC][0], texcoordAttribBytes);

    }

    unsigned short actualIndexMapIndex = 0;
    for (auto it : vertexDataMap) {
        indexDataMap[it.first] = actualIndexMapIndex;
        actualIndexMapIndex++;
        // check for overflow
        if (actualIndexMapIndex == 0) {
            // indicates that there are more than 2^16 distinct vertices,
            // so indices won't fit in an unsigned short
            __android_log_print(ANDROID_LOG_ERROR, "OBJParse", "out of indices!!!!!!!\n");
        }
        vertexData.push_back(it.second);
    }

    for (unsigned int i = 0; i < (unsigned int) obj_f.size(); i++) {
        unsigned int vertA = obj_f[i][0] - 1;
        unsigned int vertB = obj_f[i][3] - 1;
        unsigned int vertC = obj_f[i][6] - 1;

        unsigned int normA = obj_f[i][2] - 1;
        unsigned int normB = obj_f[i][5] - 1;
        unsigned int normC = obj_f[i][8] - 1;

        unsigned int texA = obj_f[i][1] - 1;
        unsigned int texB = obj_f[i][4] - 1;
        unsigned int texC = obj_f[i][7] - 1;

        VertexKey keyA = {vertA, normA, texA};
        VertexKey keyB = {vertB, normB, texB};
        VertexKey keyC = {vertC, normC, texC};

        indexData.push_back(indexDataMap[keyA]);
        indexData.push_back(indexDataMap[keyB]);
        indexData.push_back(indexDataMap[keyC]);
    }
}
