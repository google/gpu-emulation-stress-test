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

#ifndef GPU_EMULATION_STRESS_TEST_GLES3RENDERER_H
#define GPU_EMULATION_STRESS_TEST_GLES3RENDERER_H

#include "GLES2Renderer.h"

class GLES3Renderer : public GLES2Renderer {
public:
    virtual GLuint compileAndValidateShader(GLenum shaderType, const char *src);

    virtual GLuint compileShaderProgram(const char *vshaderSrc, const char *fshaderSrc,
                                        void (*preLinkFunc)(GLuint) = nullptr);

    virtual GLuint createDiffuseOnlyShaderProgram();

    virtual void reInit(WorldState *world, int width, int height);

    virtual void initRenderModel(const RenderModel &model);

    virtual void initializeRenderState(const RenderState &targetState, bool forDepth);

    virtual void changeRenderState(render_state_handle_t, bool forDepth = false);

    virtual void initShadowRendererState();

    virtual void blurPass();

    virtual void finalPass();

    virtual void initSkybox();

    virtual void renderSkybox();

    virtual void preDrawUpdate();

    virtual void draw();

    GLuint defaultVao;

    GLuint depthMapDestination;

    GLuint depthMapBlurFbo;
    GLuint depthMapBlur;
    GLuint depthMapBlurProgram;
    GLuint depthMapBlurVao;
    GLint depthMapWorldMatrixLoc;
    GLint blurProgramSamplerLoc;
    GLint blurProgramScaleLoc;

    GLuint lastSceneFbo;
    GLuint lastSceneFboColor0Texture;
    GLuint lastSceneFboColor1Velocity;
    GLuint lastSceneFboDepthTexture;

    GLint shadowRenderWindowWidthLoc;
    GLint shadowRenderWindowHeightLoc;

    matrix4 lastCameraMatrix;
    matrix4 lastCameraSkyboxMatrix;
    GLint lastCameraProjLoc;
    vector4 lastCameraPos;;
    vector4 cameraVelocity;

    GLuint finalPassProgram;
    GLint finalPassProgramColorSamplerLoc;
    GLint finalPassProgramVelocitySamplerLoc;
    GLint finalPassProgramTestBlurDirLoc;

    GLint finalPassProgramWindowWidthLoc;
    GLint finalPassProgramWindowHeightLoc;
};


#endif //GPU_EMULATION_STRESS_TEST_GLES3RENDERER_H
