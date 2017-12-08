// Copyright 2017 Lingfeng Yang
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// 
// 1. Redistributions of source code must retain the above copyright notice, this
// list of conditions and the following disclaimer.
// 
// 2. Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
// 
// 3. Neither the name of the copyright holder nor the names of its contributors
// may be used to endorse or promote products derived from this software without
// specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
//         SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
// OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "matrix.h"

#define _USE_MATH_DEFINES

#include "math.h"

#include <string>
#include <sstream>

float pi = (float) M_PI;
float pi2 = (float) M_PI_2;
float pi4 = (float) M_PI_4;

matrix4 makematrix4(
        float x00, float x01, float x02, float x03,
        float x10, float x11, float x12, float x13,
        float x20, float x21, float x22, float x23,
        float x30, float x31, float x32, float x33) {
    matrix4 res;
    res.vals[0] = x00;
    res.vals[4] = x01;
    res.vals[8] = x02;
    res.vals[12] = x03;
    res.vals[1] = x10;
    res.vals[5] = x11;
    res.vals[9] = x12;
    res.vals[13] = x13;
    res.vals[2] = x20;
    res.vals[6] = x21;
    res.vals[10] = x22;
    res.vals[14] = x23;
    res.vals[3] = x30;
    res.vals[7] = x31;
    res.vals[11] = x32;
    res.vals[15] = x33;
    return res;
}

vector4 makevector4(float x, float y, float z, float w) {
    vector4 res;
    res.x = x;
    res.y = y;
    res.z = z;
    res.w = w;
    return res;
}

matrix4 identity4() {
    return makematrix4(
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f);
}

matrix4 translation(float x, float y, float z) {
    return makematrix4(
            1.0f, 0.0f, 0.0f, x,
            0.0f, 1.0f, 0.0f, y,
            0.0f, 0.0f, 1.0f, z,
            0.0f, 0.0f, 0.0f, 1.0f);
}

matrix4 translation(const vector4 &v) {
    return translation(v.x, v.y, v.z);
}

matrix4 scaling(float x, float y, float z) {
    return makematrix4(
            x, 0.0f, 0.0f, 0.0f,
            0.0f, y, 0.0f, 0.0f,
            0.0f, 0.0f, z, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f);
}

matrix4 rotation(float ax, float ay, float az, float angle) {
    float x = ax * sinf(angle / 2);
    float y = ay * sinf(angle / 2);
    float z = az * sinf(angle / 2);
    float w = cosf(angle / 2);

    float _2x2 = 2 * x * x;
    float _2y2 = 2 * y * y;
    float _2z2 = 2 * z * z;

    float _2xy = 2 * x * y;
    float _2yz = 2 * y * z;
    float _2xz = 2 * x * z;

    float _2wx = 2 * w * x;
    float _2wy = 2 * w * y;
    float _2wz = 2 * w * z;

    return makematrix4(
            1.0f - _2y2 - _2z2, _2xy - _2wz, _2xz + _2wy, 0.0f,
            _2xy + _2wz, 1.0f - _2x2 - _2z2, _2yz - _2wx, 0.0f,
            _2xz - _2wy, _2yz + _2wx, 1.0f - _2x2 - _2y2, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f);
}

vector4 vzero4() {
    return makevector4(0.0f, 0.0f, 0.0f, 0.0f);
}

float v4dot(const vector4 &a, const vector4 &b) {
    float res = 0.0f;
    res += a.x * b.x;
    res += a.y * b.y;
    res += a.z * b.z;
    res += a.w * b.w;
    return res;
}

float v4dot3d(const vector4 &a, const vector4 &b) {
    float res = 0.0f;
    res += a.x * b.x;
    res += a.y * b.y;
    res += a.z * b.z;
    return res;
}

float velt(const vector4 &v, int i) {
    switch (i) {
        case 0:
            return v.x;
        case 1:
            return v.y;
        case 2:
            return v.z;
        case 3:
            return v.w;
    }
    return v.x;
}

vector4 v4cross(const vector4 &a, const vector4 &b) {
    return makevector4(
            velt(a, 1) * velt(b, 2) - velt(a, 2) * velt(b, 1),
            velt(a, 2) * velt(b, 0) - velt(a, 0) * velt(b, 2),
            velt(a, 0) * velt(b, 1) - velt(a, 1) * velt(b, 0),
            1.0f); // discard w for now
}

vector4 mrow(const matrix4 &m, int i) {
    switch (i) {
        case 0:
            return makevector4(m.vals[0], m.vals[4], m.vals[8], m.vals[12]);
        case 1:
            return makevector4(m.vals[1], m.vals[5], m.vals[9], m.vals[13]);
        case 2:
            return makevector4(m.vals[2], m.vals[6], m.vals[10], m.vals[14]);
        case 3:
            return makevector4(m.vals[3], m.vals[7], m.vals[11], m.vals[15]);
        default:
            return vzero4();
    }
}

vector4 mcol(const matrix4 &m, int i) {
    switch (i) {
        case 0:
            return makevector4(m.vals[0], m.vals[1], m.vals[2], m.vals[3]);
        case 1:
            return makevector4(m.vals[4], m.vals[5], m.vals[6], m.vals[7]);
        case 2:
            return makevector4(m.vals[8], m.vals[9], m.vals[10], m.vals[11]);
        case 3:
            return makevector4(m.vals[12], m.vals[13], m.vals[14], m.vals[15]);
        default:
            return vzero4();
    }
}

float melt(const matrix4 &m, int row, int col) {
    return m.vals[col * 4 + row];
}

matrix4 multm4(const matrix4 &a, const matrix4 &b) {
    return makematrix4(
            v4dot(mrow(a, 0), mcol(b, 0)), v4dot(mrow(a, 0), mcol(b, 1)),
            v4dot(mrow(a, 0), mcol(b, 2)), v4dot(mrow(a, 0), mcol(b, 3)),
            v4dot(mrow(a, 1), mcol(b, 0)), v4dot(mrow(a, 1), mcol(b, 1)),
            v4dot(mrow(a, 1), mcol(b, 2)), v4dot(mrow(a, 1), mcol(b, 3)),
            v4dot(mrow(a, 2), mcol(b, 0)), v4dot(mrow(a, 2), mcol(b, 1)),
            v4dot(mrow(a, 2), mcol(b, 2)), v4dot(mrow(a, 2), mcol(b, 3)),
            v4dot(mrow(a, 3), mcol(b, 0)), v4dot(mrow(a, 3), mcol(b, 1)),
            v4dot(mrow(a, 3), mcol(b, 2)), v4dot(mrow(a, 3), mcol(b, 3)));
}

matrix4 operator*(const matrix4 &a, const matrix4 &b) {
    return multm4(a, b);
}

vector4 multm4v4(const matrix4 &a, const vector4 &b) {
    return makevector4(
            v4dot(mrow(a, 0), b),
            v4dot(mrow(a, 1), b),
            v4dot(mrow(a, 2), b),
            v4dot(mrow(a, 3), b));
}

vector4 operator*(const matrix4 &a, const vector4 &b) {
    return multm4v4(a, b);
}

vector4 multv4m4(const vector4 &b, const matrix4 &a) {
    return makevector4(
            v4dot(mcol(a, 0), b),
            v4dot(mcol(a, 1), b),
            v4dot(mcol(a, 2), b),
            v4dot(mcol(a, 3), b));
}

vector4 operator*(const vector4 &a, const matrix4 &b) {
    return multv4m4(a, b);
}

matrix4 translated(const matrix4 &m, float x, float y, float z) {
    vector4 tv = makevector4(x, y, z, 1.0f);
    return makematrix4(
            melt(m, 0, 0), melt(m, 0, 1), melt(m, 0, 2), v4dot(mrow(m, 0), tv),
            melt(m, 1, 0), melt(m, 1, 1), melt(m, 1, 2), v4dot(mrow(m, 1), tv),
            melt(m, 2, 0), melt(m, 2, 1), melt(m, 2, 2), v4dot(mrow(m, 2), tv),
            0.0f, 0.0f, 0.0f, 1.0f);
}

// 3D length
float v4len(const vector4 &v) {
    float total = 0.0f;
    float x = v.x;
    float y = v.y;
    float z = v.z;
    total += x * x;
    total += y * y;
    total += z * z;
    total = sqrtf(total);
    return total;
}

vector4 v4normed(const vector4 &v) {
    float l = v4len(v);
    if (!l) return v;
    return makevector4(
            v.x / l,
            v.y / l,
            v.z / l,
            v.w);
}

vector4 v4stretch(const vector4 &v, float wantedLen) {
    vector4 n = v4normed(v);
    return wantedLen * n;
}


vector4 operator+(const vector4 &a, const vector4 &b) {
    return makevector4(
            a.x + b.x,
            a.y + b.y,
            a.z + b.z,
            a.w + b.w);
}

vector4 operator-(const vector4 &a, const vector4 &b) {
    return makevector4(
            a.x - b.x,
            a.y - b.y,
            a.z - b.z,
            a.w - b.w);
}

vector4 operator*(float x, const vector4 &a) {
    return makevector4(
            a.x * x,
            a.y * x,
            a.z * x,
            a.w * x);
}

vector4 operator*(const vector4 &a, float x) {
    return x * a;
}

vector4 operator/(const vector4 &a, float x) {
    return makevector4(
            a.x / x,
            a.y / x,
            a.z / x,
            a.w / x);
}

std::string dumpv4(const vector4 &a) {
    std::ostringstream ss;
    ss << "v4[ ";
    ss << " " << a.x << " ";
    ss << " " << a.y << " ";
    ss << " " << a.z << " ";
    ss << " " << a.w << " ";
    ss << "]";
    return ss.str();
}

std::string dumpm4(const matrix4 &m) {
    std::ostringstream ss;
    ss << "m4[ " << m.vals[0] << " " << m.vals[4] << " " << m.vals[8] << " " << m.vals[12] << "\n";
    ss << "    " << m.vals[1] << " " << m.vals[5] << " " << m.vals[9] << " " << m.vals[13] << "\n";
    ss << "    " << m.vals[2] << " " << m.vals[6] << " " << m.vals[10] << " " << m.vals[14] << "\n";
    ss << "    " << m.vals[3] << " " << m.vals[7] << " " << m.vals[11] << " " << m.vals[15] << "\n";
    return ss.str();
}

rayv4 makerayv4(const vector4 &pos, const vector4 &dir) {
    rayv4 res;
    res.pos = pos;
    res.dir = dir;
    return res;
}

vector4 proj_pt_to_ray(const vector4 &pt, const rayv4 &line) {
    vector4 pt2pos = pt - line.pos;
    float projfactor = v4dot3d(pt2pos, line.dir);
    vector4 tr = projfactor * line.dir;
    return translation(tr) * line.pos;
}

float proj_pt_to_ray_coord(const vector4 &pt, const rayv4 &line) {
    vector4 pt2pos = pt - line.pos;
    float projfactor = v4dot3d(pt2pos, v4normed(line.dir));
    return projfactor;
}

interval2 makeinterval2(float a, float b) {
    interval2 res;
    res.a = a;
    res.b = b;
    return res;
}

interval2 makeinterval2(std::vector<float> xs) {
    float a = xs[0];
    float b = a;

    for (auto x : xs) {
        if (x < a) a = x;
        if (x > b) b = x;
    }
    return makeinterval2(a, b);
}

bool overlapinterval2(const interval2 &a, const interval2 &b) {
    return !(a.b <= b.a || b.b <= a.a);
}

std::string dumpinterval2(const interval2 &i) {
    std::ostringstream ss;
    ss << "i2[ " << i.a << " " << i.b << " ]";
    return ss.str();
}

matrix4 makePerspectiveProj(
        float horiz_fov, // degrees;
        float aspect, // aspect ratio
        float n,
        float f) {
    float fov_rad =
            horiz_fov * pi / 180.0f;

    // l / t = aspect,
    // horiz_fov = 2 * arctan(l / f)
    // l / f = tan (horiz_fov / 2)

    float l = -n * tanf(0.5f * fov_rad);
    float r = -l;

    float t = r / aspect;
    float b = -t;

    return makematrix4(
            2.0f * n / (r - l), 0.0f, (r + l) / (r - l), 0.0f,
            0.0f, 2.0f * n / (t - b), (t + b) / (t - b), 0.0f,
            0.0f, 0.0f, -(f + n) / (f - n), -2.0f * f * n / (f - n),
            0.0f, 0.0f, -1.0f, 0.0f);
}

matrix4 makeOrthogonalProj(
        float right, float aspect,
        float n, float f) {

    float top = right / aspect;
    return makematrix4(
            1.0f / right, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f / top, 0.0f, 0.0f,
            0.0f, 0.0f, -2.0f / (f - n), -(f + n) / (f - n),
            0.0f, 0.0f, 0.0f, 1.0f);
}

matrix4 makeFrameChange(
        const vector4 &position,
        const vector4 &dir,
        const vector4 &up) {
    vector4 coordB = v4normed(up);
    vector4 coordC = -1.0f * v4normed(
            dir); // |dir| points in -z dir only for humans, for math and opengl, |coordC| needs to specify +z dir.
    vector4 coordA = v4normed(v4cross(coordB, coordC));
    coordB = v4cross(coordC, coordA);

    matrix4 coords =
            makematrix4(
                    coordA.x, coordB.x, coordC.x, 0.0f,
                    coordA.y, coordB.y, coordC.y, 0.0f,
                    coordA.z, coordB.z, coordC.z, 0.0f,
                    0.0f, 0.0f, 0.0f, 1.0f);

    matrix4 tr =
            makematrix4(
                    1.0f, 0.0f, 0.0f, position.x,
                    0.0f, 1.0f, 0.0f, position.y,
                    0.0f, 0.0f, 1.0f, position.z,
                    0.0f, 0.0f, 0.0f, 1.0f);

    return tr * coords;
}

matrix4 makeModelview(
        const vector4 &position,
        const vector4 &dir,
        const vector4 &up) {
    vector4 coordB = v4normed(up);
    vector4 coordC = -1.0f * v4normed(dir);
    vector4 coordA = v4normed(v4cross(coordB, coordC));
    coordB = v4cross(coordC, coordA);

    // Like makeFrameChange, but do the inverse transformation.
    matrix4 coords =
            makematrix4(
                    coordA.x, coordA.y, coordA.z, 0.0f,
                    coordB.x, coordB.y, coordB.z, 0.0f,
                    coordC.x, coordC.y, coordC.z, 0.0f,
                    0.0f, 0.0f, 0.0f, 1.0f);

    matrix4 tr =
            makematrix4(
                    1.0f, 0.0f, 0.0f, -position.x,
                    0.0f, 1.0f, 0.0f, -position.y,
                    0.0f, 0.0f, 1.0f, -position.z,
                    0.0f, 0.0f, 0.0f, 1.0f);

    return coords * tr;
}

matrix4 makeCameraMatrix(
        const vector4 &position,
        const vector4 &dir,
        const vector4 &up,
        float fov,
        float aspect,
        float nearclip, float farclip) {

    return makePerspectiveProj(fov, aspect, nearclip, farclip) *
           makeModelview(position, dir, up);
}

matrix4 transpose(
        const matrix4 &m) {
    return
            makematrix4(
                    m.vals[0], m.vals[1], m.vals[2], m.vals[3],
                    m.vals[4], m.vals[5], m.vals[6], m.vals[7],
                    m.vals[8], m.vals[9], m.vals[10], m.vals[11],
                    m.vals[12], m.vals[13], m.vals[14], m.vals[15]);
}

matrix4 makeInverseModelview(
        const vector4 &position,
        const vector4 &dir,
        const vector4 &up) {
    vector4 coordB = v4normed(up);
    vector4 coordC = -1.0f * v4normed(dir);
    vector4 coordA = v4normed(v4cross(coordB, coordC));
    coordB = v4cross(coordC, coordA);

    matrix4 coordsI =
            transpose(
                    makematrix4(
                            coordA.x, coordA.y, coordA.z, 0.0f,
                            coordB.x, coordB.y, coordB.z, 0.0f,
                            coordC.x, coordC.y, coordC.z, 0.0f,
                            0.0f, 0.0f, 0.0f, 1.0f));

    matrix4 trI =
            makematrix4(
                    1.0f, 0.0f, 0.0f, position.x,
                    0.0f, 1.0f, 0.0f, position.y,
                    0.0f, 0.0f, 1.0f, position.z,
                    0.0f, 0.0f, 0.0f, 1.0f);

    return trI * coordsI;
}
