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

#ifndef GPU_EMULATION_STRESS_TEST_ACTIONCURVE_H
#define GPU_EMULATION_STRESS_TEST_ACTIONCURVE_H

#include <map>
#include <string>
#include <vector>

class ActionCurve {
public:
    enum class CurveType {
        Bezier = 0,
        Exponential = 1,
    };

    enum class KeyHandleType {
        AutoClamped = 0,
    };

    struct Keyframe {
        CurveType curveType;
        KeyHandleType leftHandleType;
        KeyHandleType rightHandleType;
        float hlx;
        float hly;
        float hrx;
        float hry;
        float x;
        float y;
    };

    ActionCurve() = default;

    static CurveType curveTypeFromString(const std::string &str) {
        if (str == "BEZIER") return CurveType::Bezier;
        if (str == "EXPO") return CurveType::Exponential;
        return CurveType::Bezier;
    }

    static KeyHandleType handleTypeFromString(const std::string &str) {
        if (str == "AUTO_CLAMPED") return KeyHandleType::AutoClamped;
        return KeyHandleType::AutoClamped;
    }

    void addKey(CurveType curveType,
                KeyHandleType leftType,
                KeyHandleType rightType,
                float hlx, float hly,
                float hrx, float hry,
                float x, float y) {
        Keyframe kf;
        kf.curveType = curveType;
        kf.leftHandleType = leftType;
        kf.rightHandleType = rightType;

        kf.hlx = hlx;
        kf.hly = hly;
        kf.hrx = hrx;
        kf.hry = hry;
        kf.x = x;
        kf.y = y;

        mKeyframes.push_back(kf);
        refresh();
    }

    float evalAtFrame(int frame, bool normalized);

private:

    void refresh();

    // float getYValAt(int keyframeIndex, float offset);

    float mMinY;
    float mMaxY;

    std::vector<Keyframe> mKeyframes;
    std::vector<size_t> mLUT;
};


#endif //GPU_EMULATION_STRESS_TEST_ACTIONCURVE_H
