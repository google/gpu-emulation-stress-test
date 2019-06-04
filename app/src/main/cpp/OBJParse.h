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

#ifndef GPU_EMULATION_STRESS_TEST_OBJPARSE_H
#define GPU_EMULATION_STRESS_TEST_OBJPARSE_H

#include <string>
#include <map>
#include <vector>

class OBJParse {
public:
    OBJParse() {}

    OBJParse(const std::string &objFileName);

    // Use interleaved vertex attributes
    struct VertexAttributes {
        float pos[3];
        float norm[3];
        float texcoord[2];
    };

    struct VertexKey {
        unsigned int pos;
        unsigned int norm;
        unsigned int texcoord;
    };

    struct VertexKeyCompare {
        bool operator()(const VertexKey &a,
                        const VertexKey &b) const {
            if (a.pos != b.pos)
                return a.pos < b.pos;
            if (a.norm != b.norm)
                return a.norm < b.norm;
            if (a.texcoord != b.texcoord)
                return a.texcoord < b.texcoord;
            return false;
        }
    };

    std::vector<VertexAttributes> vertexData;
    std::vector<unsigned short> indexData;
private:
    std::vector<std::vector<float> > obj_v;
    std::vector<std::vector<float> > obj_vn;
    std::vector<std::vector<float> > obj_vt;
    std::vector<std::vector<uint32_t> > obj_f;

    std::map<VertexKey, VertexAttributes, VertexKeyCompare> vertexDataMap;
    std::map<VertexKey, unsigned short, VertexKeyCompare> indexDataMap;
};


#endif //GPU_EMULATION_STRESS_TEST_OBJPARSE_H
