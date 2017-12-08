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

#pragma once

#include <string>
#include <vector>

extern float pi;
struct matrix4 {
    float vals[16];
};
struct vector4 {
    float x;
    float y;
    float z;
    float w;
};

matrix4 makematrix4(
        float x00, float x01, float x02, float x03,
        float x10, float x11, float x12, float x13,
        float x20, float x21, float x22, float x23,
        float x30, float x31, float x32, float x33);

vector4 makevector4(float x, float y, float z, float w);

matrix4 identity4();

matrix4 translation(float x, float y, float z);

matrix4 translation(const vector4 &v);

matrix4 scaling(float x, float y, float z);

matrix4 rotation(float ax, float ay, float az, float angle);

vector4 vzero4();

float v4dot(const vector4 &a, const vector4 &b);

float v4dot3d(const vector4 &a, const vector4 &b);

float velt(const vector4 &v, int i);

vector4 v4cross(const vector4 &a, const vector4 &b);

vector4 mrow(const matrix4 &m, int i);

vector4 mcol(const matrix4 &m, int i);

float melt(const matrix4 &m, int row, int col);

matrix4 multm4(const matrix4 &a, const matrix4 &b);

matrix4 operator*(const matrix4 &a, const matrix4 &b);

vector4 multm4v4(const matrix4 &a, const vector4 &b);

vector4 operator*(const matrix4 &a, const vector4 &b);

vector4 multv4m4(const vector4 &b, const matrix4 &a);

vector4 operator*(const vector4 &a, const matrix4 &b);

matrix4 translated(const matrix4 &m, float x, float y, float z);

float v4len(const vector4 &v);

vector4 v4normed(const vector4 &v);

vector4 v4stretch(const vector4 &v, float wantedLen);

vector4 operator+(const vector4 &a, const vector4 &b);

vector4 operator-(const vector4 &a, const vector4 &b);

vector4 operator*(float x, const vector4 &a);

vector4 operator*(const vector4 &a, float x);

vector4 operator/(const vector4 &a, float x);

std::string dumpv4(const vector4 &a);

std::string dumpm4(const matrix4 &m);

struct rayv4 {
    vector4 pos;
    vector4 dir;
};

rayv4 makerayv4(const vector4 &pos, const vector4 &dir);

vector4 proj_pt_to_ray(const vector4 &pt, const rayv4 &line);

float proj_pt_to_ray_coord(const vector4 &pt, const rayv4 &line);

struct interval2 {
    float a;
    float b;
};

interval2 makeinterval2(float a, float b);

interval2 makeinterval2(std::vector<float> xs);

bool overlapinterval2(const interval2 &a, const interval2 &b);

std::string dumpinterval2(const interval2 &i);

matrix4 makePerspectiveProj(
        float horiz_fov, // degrees;
        float aspect, // aspect ratio
        float n,
        float f);

matrix4 makeOrthogonalProj(
        float right, // ortho scale
        float aspect, // aspect ratio
        float n,
        float f);

matrix4 makeFrameChange(
        const vector4 &position,
        const vector4 &dir,
        const vector4 &up);

matrix4 makeModelview(
        const vector4 &position,
        const vector4 &dir,
        const vector4 &up);

matrix4 makeCameraMatrix(
        const vector4 &position,
        const vector4 &dir,
        const vector4 &up,
        float fov, float aspect, float nearclip, float farclip);

matrix4 transpose(const matrix4 &m);

matrix4 makeInverseModelview(
        const vector4 &position,
        const vector4 &dir,
        const vector4 &up);

