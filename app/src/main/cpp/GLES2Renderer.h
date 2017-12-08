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

#ifndef GPU_EMULATION_STRESS_TEST_GLES2RENDERER_H
#define GPU_EMULATION_STRESS_TEST_GLES2RENDERER_H

#include "WorldState.h"

#ifdef DESKTOP_GL
#include "DesktopGLHeaders.h"
#else

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <GLES3/gl3.h>
#include <GLES3/gl31.h>

#endif

#include <vector>
#include <unordered_set>

class GLES2Renderer {
public:
    int windowWidth;
    int windowHeight;

    virtual GLuint compileAndValidateShader(GLenum shaderType, const char *src);

    virtual GLuint compileShaderProgram(const char *vshaderSrc, const char *fshaderSrc,
                                        void (*preLinkFunc)(GLuint) = nullptr);

    virtual GLuint createDiffuseOnlyShaderProgram();

    virtual void reInit(WorldState *world, int width, int height);

    virtual void initRenderModel(const RenderModel &model);

    virtual void preDrawUpdate();

    virtual void draw();

    WorldState *world;

    matrix4 currentCameraMatrix;
    matrix4 currentCameraSkyboxMatrix;

    struct RenderState {
        GLuint shaderProgram;
        GLint uWorldMatrixLoc;
        GLint uCameraMatrixLoc;
        GLint aPosLoc;
        GLint aNormLoc;
        GLint aTexcoordLoc;

        GLuint vbo;
        GLuint ibo;

        // Textures by unit
        GLuint texture0;
        GLuint texture1;

        // GLES3 stuff
        GLuint vao;
        GLint uWorldMatrixPrevLoc;
        GLint uCameraMatrixPrevLoc;
    };

    virtual void initializeRenderState(const RenderState &targetState, bool forDepth);

    virtual void updateVertexAttributes(const RenderState &targetState);

    virtual void changeRenderState(render_state_handle_t, bool forDepth = false);

    bool renderStateInitialized = false;
    RenderState currRenderState;
    std::vector<RenderState> renderStates;

    bool shadowMapsEnabled = true;

    virtual void initShadowRendererState();

    // Shadow map render state
    matrix4 currentLightMatrix;

    GLuint depthMapProgram;
    GLuint shadowRenderProgram;
    GLint shadowLightPosUniformLoc;
    float shadowLightPos_x;
    float shadowLightPos_y;
    float shadowLightPos_z;

    GLuint depthMapFbo;
    GLuint depthMapTexture;

    // Skybox render state
    bool hasSkybox;
    GLuint skyboxProgram;
    GLuint skyboxTexture;
    GLint skyboxAttribLoc;
    GLint skyboxMatrixUniformLoc;
    GLint skyboxSamplerUniformLoc;
    GLuint skyboxVbo;
    GLuint skyboxVao;

    virtual void initSkybox();

    virtual void renderSkybox();

    virtual void setModelVertexAttribs();

    virtual void setSkyboxVertexAttribs();

    // hopefully, we can sort objects so that
    // render state doesn't change very much.
    struct ObjectState {
        bool visible;
        render_state_handle_t renderHandle;
        uint32_t indexCount;
        matrix4 worldMatrix;
        matrix4 lastWorldMatrix; // for motion blur
    };

    std::vector<ObjectState> objects;
};


#endif //GPU_EMULATION_STRESS_TEST_GLES2RENDERER_H
