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

#include "ActionCurve.h"

#include "log.h"

#include <algorithm>

struct order_by_frame {
    bool operator()(const ActionCurve::Keyframe &l, const ActionCurve::Keyframe &r) const {
        return l.x < r.x;
    }
};

void ActionCurve::refresh() {
    if (mKeyframes.empty()) return;
    if (mKeyframes.size() < 2) return;

    std::sort(mKeyframes.begin(), mKeyframes.end(), order_by_frame());
    int maxFrame = (int) mKeyframes[mKeyframes.size() - 1].x;
    mLUT.resize(maxFrame, 0);

    float minY = mKeyframes[0].y;
    float maxY = minY;

    for (const auto &elt : mKeyframes) {
        if (elt.y < minY) minY = elt.y;
        if (elt.y > maxY) maxY = elt.y;
    }

    mMinY = minY;
    mMaxY = maxY;

    for (int i = 0; i < mLUT.size(); i++) {
        for (int j = (i > 0 ? mLUT[i - 1] : 0);
             j < mKeyframes.size() - 1; j++) {
            if (i >= (int) mKeyframes[j].x &&
                i < (int) mKeyframes[j + 1].x) {
                mLUT[i] = j;
            }
        }
    }
}

// TODO: get the real parameterization
float ActionCurve::evalAtFrame(int frame, bool normalized) {
    const Keyframe &a = mKeyframes[mLUT[frame]];
    const Keyframe &b = mKeyframes[mLUT[frame] + 1];

    float df = frame - a.x;
    float total = b.x - a.x;

    float t = df / total;
    float tm = 1.0f - t;

    float c0 = tm * tm * tm;
    float c1 = t * tm * tm;
    float c2 = t * t * tm;
    float c3 = t * t * t;

    float yres = c0 * a.y + c1 * a.hry + c2 * b.hly + c3 * b.y;

    if (normalized) {
        yres = (yres - mMinY) / (mMaxY - mMinY);
    }

    return yres;
}
