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

