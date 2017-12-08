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

#include "BezierCurve.h"

#include "log.h"
#include "math.h"

#include <algorithm>
#include <assert.h>

void BezierCurve::evalSimple(float t, float *x_out, float *y_out, float *z_out) {
    size_t pt = (size_t) (t * (mPoints.size() - 1));

    assert(pt < mPoints.size());

    float ts = t * (mPoints.size() - 1);
    ts = ts - int(ts);


    evalElement(pt, ts, x_out, y_out, z_out);
}

void BezierCurve::evalArclen(float t, float *x_out, float *y_out, float *z_out) {
    evalSimple(arcLengthIndexed(t), x_out, y_out, z_out);
}

void BezierCurve::evalElement(size_t pt, float t, float *x_out, float *y_out, float *z_out) {
    const BezierPoint &a = mPoints[pt];
    const BezierPoint &b = mPoints[pt + 1];

    float TA = 1.0f - t;
    float TA2 = TA * TA;
    float TA3 = TA2 * TA;

    float TB = t;
    float TB2 = TB * TB;
    float TB3 = TB2 * TB;

    float c0 = TA3;
    float c1 = 3.0f * TA2 * TB;
    float c2 = 3.0f * TA * TB2;
    float c3 = TB3;

    *x_out = c0 * a.coord[0] +
             c1 * a.right[0] +
             c2 * b.left[0] +
             c3 * b.coord[0];

    *y_out = c0 * a.coord[1] +
             c1 * a.right[1] +
             c2 * b.left[1] +
             c3 * b.coord[1];

    *z_out = c0 * a.coord[2] +
             c1 * a.right[2] +
             c2 * b.left[2] +
             c3 * b.coord[2];
}

static float segLength(float x1, float y1, float z1,
                       float x2, float y2, float z2) {
    float dx = x2 - x1;
    float dy = y2 - y1;
    float dz = z2 - z1;

    float dx2 = dx * dx;
    float dy2 = dy * dy;
    float dz2 = dz * dz;

    return sqrtf(dx2 + dy2 + dz2);
}

static bool aboutEq(float x, float y) {
    return fabs(x - y) < 0.1 * x;
}

void BezierCurve::precalcArclengths(int power) {
    mPower = power;

    int count = 1 << power;

    float lastx, lasty, lastz;
    float x, y, z;
    float totalArc = 0.0f;

    std::vector<float> rawLengths;
    for (int i = 0; i < count; i++) {
        if (i == 0) {
            rawLengths.push_back(0.0f);
            evalSimple(0.0f, &lastx, &lasty, &lastz);
        } else {
            evalSimple((float) i / ((float) count), &x, &y, &z);
            float len = segLength(lastx, lasty, lastz,
                                  x, y, z);
            lastx = x;
            lasty = y;
            lastz = z;

            totalArc += len;
            float nextLen = len + rawLengths[i - 1];
            if (i % 1000 == 0) {
                LOGV("%s: nextLen %f", __func__, nextLen);
            }
            rawLengths.push_back(nextLen);
        }
    }

    LOGV("%s: total arc length %f", __func__, totalArc);

    for (int i = 0; i < count; i++) {
        rawLengths[i] /= totalArc;
    }

    mPrecalcArclengthLUT.resize(count, 0.0f);
    for (int i = 0; i < count; i++) {
        int bucket1 = int(rawLengths[i] * (count - 1));
        mPrecalcArclengthLUT[bucket1] = (float) i / (float) count;
    }

    for (int i = 0; i < count; i++) {
        if (i == 0) continue;

        float candidate = mPrecalcArclengthLUT[i];

        if (candidate) continue;

        int left = i - 1;
        int right = i + 1;
        while (left >= 0 && !mPrecalcArclengthLUT[left]) {
            left--;
        }
        while (right && right < count && !mPrecalcArclengthLUT[right]) {
            right++;
        }

        float newLeft = mPrecalcArclengthLUT[left];
        float newRight = mPrecalcArclengthLUT[right];
        if (!newRight) newRight = 1.0f;

        float leftWeight = (i - left);
        float rightWeight = (right - i);
        float lrTotal = leftWeight + rightWeight;
        leftWeight /= lrTotal;
        rightWeight /= lrTotal;

        mPrecalcArclengthLUT[i] = leftWeight * newLeft + rightWeight * newRight;
    }
}

float BezierCurve::arcLengthIndexed(float s) {
    if (!s) return 0.0f;

    int count = 1 << mPower;

    float rawIndex = s * (float) count;

    int index = int(rawIndex);

    float offset = rawIndex - (float) index;

    // int leftTap1 = (index - 1) > 0 ? (index - 1) : 0;
    int rightTap0 = (index + 1) < count ? (index + 1) : (count - 1);

    return (1.0f - offset) * mPrecalcArclengthLUT[index] + offset * mPrecalcArclengthLUT[rightTap0];

    // int rightTap1 = (index + 2) < count ? (index + 2) : (count - 1);

    // float t = 0.33333333f + offset / 3.0f;

    // float tm = 1.0f - t;

    // float k0 = tm * tm * tm;
    // float k1 = t * tm * tm;
    // float k2 = t * t * tm;
    // float k3 = t * t * t;

    // float a = mPrecalcArclengthLUT[leftTap1];
    // float b = mPrecalcArclengthLUT[index];
    // float c = mPrecalcArclengthLUT[rightTap0];
    // float d = mPrecalcArclengthLUT[rightTap1];

    // float res = k0 * a + k1 * b + k2 * c + k3 * d;

    // return res;
}

// Look up table taking control point parameterization to
// arc length parameterization
// subdivide curve into line segments that all attempt to be close to segWidth
void BezierCurve::precalcWithResolution(float resolution) {

    float currBestTstep = 0.1f;

    size_t currPt = 0;
    float currT = 0.0f;
    float lastx, lasty, lastz;
    evalElement(currPt, currT, &lastx, &lasty, &lastz);
    float x, y, z;
    float resCurrent = 0.0f;

    float lastT = 0.0f;
    float delta = 0.1f;
    currT = 0.1f;
    while (true) {
        redo:
        evalElement(currPt, currT, &x, &y, &z);
        resCurrent = segLength(lastx, lasty, lastz, x, y, z);
        bool needStep = !aboutEq(resolution, resCurrent);
        if (needStep) {
            float distDiff = resolution / resCurrent;
            currT = std::min(1.0f, std::max(lastT + 0.00001f, currT * distDiff));
            goto redo;
        }

        lastx = x;
        lasty = y;
        lastz = z;
        delta = currT - lastT;
        lastT = currT;
        currT += delta;

        if (currT >= 1.0f) {
            if (currPt == mPoints.size() - 1) break;

            currT -= 1.0f;
            currPt += 1;
            LOGV("%s: currPt %zu\n", __func__, currPt);
        }
    }
}
