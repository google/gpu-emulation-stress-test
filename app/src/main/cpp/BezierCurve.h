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

#ifndef GPU_EMULATION_STRESS_TEST_BEZIERCURVE_H
#define GPU_EMULATION_STRESS_TEST_BEZIERCURVE_H

#include "ActionCurve.h"

#include "matrix.h"

#include <map>
#include <vector>

struct BezierCoord {
    float x;
    float y;
    float z;
    float arcLength;
};

struct BezierPoint {
    float left[3];
    float right[3];
    float coord[3];
};

class BezierCurve {
public:
    BezierCurve() = default;

    void addPoint(
            int index,
            float lx, float ly, float lz,
            float rx, float ry, float rz,
            float x, float y, float z) {
        BezierPoint p;
        p.left[0] = lx;
        p.left[1] = ly;
        p.left[2] = lz;
        p.right[0] = rx;
        p.right[1] = ry;
        p.right[2] = rz;
        p.coord[0] = x;
        p.coord[1] = y;
        p.coord[2] = z;

        mPoints.resize(index + 1);
        mPoints[index] = p;
    }

    void precalcArclengths(int power);

    void precalcWithResolution(float segWidth);

    void evalSimple(float t, float *x_out, float *y_out, float *z_out);

    void evalArclen(float t, float *x_out, float *y_out, float *z_out);

    ActionCurve &action() {
        return mAction;
    }

private:
    void evalElement(size_t pt, float t, float *x_out, float *y_out, float *z_out);

    std::vector<BezierPoint> mPoints;
    std::vector<BezierCoord> mPrecalculated;

    int mPower;

    float arcLengthIndexed(float t);

    std::vector<float> mPrecalcArclengthLUT;

    ActionCurve mAction;
};


#endif //GPU_EMULATION_STRESS_TEST_BEZIERCURVE_H
