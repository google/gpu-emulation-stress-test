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

#ifndef GPU_EMULATION_STRESS_TEST_ENTITY_H
#define GPU_EMULATION_STRESS_TEST_ENTITY_H

#include <cstdlib>
#include "log.h"

#include "matrix.h"

typedef uint32_t render_state_handle_t;

class Entity {
public:
    Entity() = default;

    void updateWorldMatrix(matrix4 &targetWorldMatrix) const {
        targetWorldMatrix =
                makeFrameChange(pos, v4normed(fwd), v4normed(up)) *
                scaling(scale.x, scale.y, scale.z);
    }

    void updateCameraMatrix(float fov, float aspect, float nearclip, float farclip,
                            float right, bool isOrtho,
                            matrix4 &targetCameraMatrix) const {
        matrix4 proj = isOrtho ? makeOrthogonalProj(right, aspect, nearclip, farclip) :
                       makePerspectiveProj(fov, aspect, nearclip, farclip);
        targetCameraMatrix = proj * makeModelview(pos, fwd, up);
    }

    void updateCameraViewMatrix(float fov, float aspect, float nearclip, float farclip,
                                float right, bool isOrtho,
                                matrix4 &targetCameraMatrix) const {
        targetCameraMatrix = makeModelview(pos, fwd, up);
    }

    void updateCameraProjMatrix(float fov, float aspect, float nearclip, float farclip,
                                float right, bool isOrtho,
                                matrix4 &targetCameraMatrix) const {
        matrix4 proj = isOrtho ? makeOrthogonalProj(right, aspect, nearclip, farclip) :
                       makePerspectiveProj(fov, aspect, nearclip, farclip);
        targetCameraMatrix = proj;
    }

    void updateCameraSkyboxMatrix(float fov, float aspect, float nearclip, float farclip,
                                  float right, bool isOrtho,
                                  matrix4 &targetCameraMatrix) const {
        matrix4 proj = isOrtho ? makeOrthogonalProj(right, aspect, nearclip, farclip) :
                       makePerspectiveProj(fov, aspect, nearclip, farclip);
        targetCameraMatrix = proj * makeModelview(makevector4(0, 0, 0, 1), fwd, up);
    }

    void update(uint32_t currFrame) {
        if (!frameKnown) {
            frameKnown = true;
            lastFrame = currFrame;
            if (!framesToLive) {
                LOGE("Tried to create entity with zero lifetime. No good!");
                abort();
            }
        } else {
            if (framesToLive > 0) {
                if (currFrame > lastFrame) {
                    framesToLive -= currFrame - lastFrame;
                }
                lastFrame = currFrame;
                if (framesToLive <= 0) {
                    live = false;
                }
            }
        }
    }

    bool renderable = true;
    render_state_handle_t renderModel = 0;
    vector4 pos;
    vector4 fwd;
    vector4 up;
    vector4 scale;

    // Lifetime / GC stuff
    bool frameKnown = false;
    uint32_t lastFrame = 0;
    int framesToLive = -1;
    bool live = true;

    // all other data that seems to matter
    vector4 initialOffset;
    vector4 spinAxis;
    int spinPeriod;

    void applyRotation(const vector4 &axis, float angle) {
        matrix4 current =
                makeFrameChange(pos, v4normed(fwd), v4normed(up));
        matrix4 rotated =
                rotation(axis.x, axis.y, axis.z, angle) * current;
        fwd = -1.0f * v4normed(mcol(rotated, 2));
        up = v4normed(mcol(rotated, 1));
    }
};

#endif //GPU_EMULATION_STRESS_TEST_ENTITY_H
