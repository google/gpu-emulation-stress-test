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

#include "GLES3Renderer.h"
#include "ScopedProfiler.h"

#include "log.h"

#if DESKTOP_GL
#include "GLCoreShaders.cpp"
#else

#include "GLES3Shaders.cpp"

#endif

void GLES3Renderer::reInit(WorldState *worldState, int width, int height) {

    glGenVertexArrays(1, &defaultVao);
    glBindVertexArray(defaultVao);

    windowWidth = width;
    windowHeight = height;

    glClearColor(0.2, 0.6, 0.7, 0.0);
    glViewport(0, 0, windowWidth, windowHeight);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    world = worldState;

    currentCameraMatrix = identity4();

    renderStates.clear();
    objects.clear();

    if (shadowMapsEnabled && world->lights.size() != 0) {
        initShadowRendererState();
    }

    if (world->skyboxName != "" &&
        world->skyboxTexWidth &&
        world->skyboxTexHeight) {
        hasSkybox = true;
        initSkybox();
    }

    for (const auto &model : world->renderModels) {
        initRenderModel(model);
    }
}

GLuint GLES3Renderer::compileAndValidateShader(GLenum shaderType, const char *src) {
    GLuint shader = glCreateShader(shaderType);
    glShaderSource(shader, 1, (const char *const *) &src, nullptr);
    glCompileShader(shader);

    GLint compileStatus;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compileStatus);

    if (compileStatus != GL_TRUE) {
        GLint infologLength = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infologLength);
        std::vector<char> infologBuf(infologLength + 1);
        glGetShaderInfoLog(shader, infologLength, nullptr, &infologBuf[0]);
        LOGE("fail to compile. infolog: %s",
             &infologBuf[0]);
    }

    return shader;
}

GLuint GLES3Renderer::compileShaderProgram(const char *vshaderSrc, const char *fshaderSrc,
                                           void (*preLinkFunc)(GLuint)) {

    GLuint vshader = compileAndValidateShader(GL_VERTEX_SHADER, vshaderSrc);
    GLuint fshader = compileAndValidateShader(GL_FRAGMENT_SHADER, fshaderSrc);

    GLuint program = glCreateProgram();

    glAttachShader(program, vshader);
    glAttachShader(program, fshader);

    if (preLinkFunc) preLinkFunc(program);

    glLinkProgram(program);

    GLint linkStatus;
    glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);

    if (linkStatus != GL_TRUE) {
        GLint infologLength = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infologLength);
        std::vector<char> infologBuf(infologLength + 1);
        glGetProgramInfoLog(program, infologLength, nullptr, &infologBuf[0]);
        LOGE("fail to link. infolog: %s",
             &infologBuf[0]);
    }

    return program;
}

GLuint GLES3Renderer::createDiffuseOnlyShaderProgram() {
    return compileShaderProgram(
            sDiffuseOnlyVShaderSrc,
            sDiffuseOnlyFShaderSrc,
            nullptr);
}

void GLES3Renderer::initShadowRendererState() {

    LOGV("compile depth map prog");
    depthMapProgram =
            compileShaderProgram(
                    sDepthMapVShaderSrc,
                    sDepthMapFShaderSrc,
                    nullptr);

    depthMapWorldMatrixLoc =
            glGetUniformLocation(depthMapProgram, "worldmatrix");

    LOGV("compile shadow render prog");
    shadowRenderProgram =
            compileShaderProgram(
                    sShadowRenderVShaderSrc,
                    sShadowRenderFShaderESMSrc,
                    nullptr);

    shadowLightPosUniformLoc =
            glGetUniformLocation(shadowRenderProgram, "lightPos");
    shadowRenderWindowWidthLoc =
            glGetUniformLocation(shadowRenderProgram, "windowWidth");
    shadowRenderWindowHeightLoc =
            glGetUniformLocation(shadowRenderProgram, "windowHeight");

    glUseProgram(shadowRenderProgram);

    glUniform1f(shadowRenderWindowWidthLoc, (float) windowWidth);
    glUniform1f(shadowRenderWindowHeightLoc, (float) windowHeight);

    glUseProgram(0);


    LOGV("compile shadow blur prog");
    depthMapBlurProgram =
            compileShaderProgram(
                    sBlurVShaderSrc, sBlurFShaderSrc, nullptr);

    blurProgramSamplerLoc =
            glGetUniformLocation(depthMapBlurProgram, "toBlur");
    blurProgramScaleLoc =
            glGetUniformLocation(depthMapBlurProgram, "scale");

    LOGV("blur sampler locs %d %d", blurProgramSamplerLoc, blurProgramScaleLoc);

    LOGV("compile final pass prog");

    finalPassProgram =
            compileShaderProgram(
                    sFinalPassVShaderSrc, sFinalPassFShaderSrc, nullptr);
    finalPassProgramColorSamplerLoc =
            glGetUniformLocation(finalPassProgram, "color");
    finalPassProgramVelocitySamplerLoc =
            glGetUniformLocation(finalPassProgram, "velocity");
    finalPassProgramTestBlurDirLoc =
            glGetUniformLocation(finalPassProgram, "testBlurDir");
    finalPassProgramWindowWidthLoc =
            glGetUniformLocation(finalPassProgram, "windowWidth");
    finalPassProgramWindowHeightLoc =
            glGetUniformLocation(finalPassProgram, "windowHeight");

    glUseProgram(finalPassProgram);
    glUniform1f(finalPassProgramWindowWidthLoc, (float) windowWidth);
    glUniform1f(finalPassProgramWindowHeightLoc, (float) windowHeight);
    glUseProgram(0);

    LOGV("final pass loc %d %d %d",
         finalPassProgramColorSamplerLoc,
         finalPassProgramVelocitySamplerLoc,
         finalPassProgramTestBlurDirLoc);

    LOGV("init shadow map fbo");

    // create depth texture and color-renderable float buffer
    {
        glGenTextures(1, &depthMapTexture);
        glBindTexture(GL_TEXTURE_2D, depthMapTexture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glTexImage2D(GL_TEXTURE_2D, 0,
                     GL_DEPTH_COMPONENT32F, kShadowMapW, kShadowMapH, 0,
                     GL_DEPTH_COMPONENT, GL_FLOAT, 0);

        glGenTextures(1, &depthMapDestination);
        glBindTexture(GL_TEXTURE_2D, depthMapDestination);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glTexImage2D(GL_TEXTURE_2D, 0,
                     GL_R16F, kShadowMapW, kShadowMapH, 0,
                     GL_RED, GL_FLOAT, 0);

        glGenTextures(1, &depthMapBlur);
        glBindTexture(GL_TEXTURE_2D, depthMapBlur);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glTexImage2D(GL_TEXTURE_2D, 0,
                     GL_R16F, kShadowMapW, kShadowMapH, 0,
                     GL_RED, GL_FLOAT, 0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    // create lastScene FBO textures
    {
        glGenTextures(1, &lastSceneFboColor0Texture);
        glBindTexture(GL_TEXTURE_2D, lastSceneFboColor0Texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glTexImage2D(GL_TEXTURE_2D, 0,
                     GL_RGBA8, windowWidth, windowHeight, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, 0);

        glGenTextures(1, &lastSceneFboColor1Velocity);
        glBindTexture(GL_TEXTURE_2D, lastSceneFboColor1Velocity);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glTexImage2D(GL_TEXTURE_2D, 0,
                     GL_RG16F, windowWidth, windowHeight, 0,
                     GL_RG, GL_FLOAT, 0);

        glGenTextures(1, &lastSceneFboDepthTexture);
        glBindTexture(GL_TEXTURE_2D, lastSceneFboDepthTexture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glTexImage2D(GL_TEXTURE_2D, 0,
                     GL_DEPTH_COMPONENT32F, windowWidth, windowHeight, 0,
                     GL_DEPTH_COMPONENT, GL_FLOAT, 0);

        glBindTexture(GL_TEXTURE_2D, 0);
    }

    { // create depth map FBO
        glGenFramebuffers(1, &depthMapFbo);
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMapTexture,
                               0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                               depthMapDestination, 0);

        int fboCompleteness = glCheckFramebufferStatus(GL_FRAMEBUFFER);

        if (fboCompleteness != GL_FRAMEBUFFER_COMPLETE) {
            LOGE("depth map fbo not complete :( 0x%x", fboCompleteness);
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    { // create blur fbo
        glGenFramebuffers(1, &depthMapBlurFbo);
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapBlurFbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, depthMapBlur,
                               0);

        int fboCompleteness = glCheckFramebufferStatus(GL_FRAMEBUFFER);

        if (fboCompleteness != GL_FRAMEBUFFER_COMPLETE) {
            LOGE("blur fbo not complete :( 0x%x", fboCompleteness);
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    { // create lastScene fbo
        glGenFramebuffers(1, &lastSceneFbo);
        glBindFramebuffer(GL_FRAMEBUFFER, lastSceneFbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                               lastSceneFboColor0Texture, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D,
                               lastSceneFboColor1Velocity, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
                               lastSceneFboDepthTexture, 0);
        const GLenum drawBufs[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
        glDrawBuffers(2, drawBufs);

        int fboCompleteness = glCheckFramebufferStatus(GL_FRAMEBUFFER);

        if (fboCompleteness != GL_FRAMEBUFFER_COMPLETE) {
            LOGE("lastScene fbo not complete :( 0x%x", fboCompleteness);
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
}

void GLES3Renderer::blurPass() {
    glDisable(GL_DEPTH_TEST);

    glUseProgram(depthMapBlurProgram);
    glBindVertexArray(defaultVao);

    glActiveTexture(GL_TEXTURE0);
    glUniform1i(blurProgramSamplerLoc, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, depthMapBlurFbo);
    glBindTexture(GL_TEXTURE_2D, depthMapDestination);
    glUniform2f(blurProgramScaleLoc, 2.0f / kShadowMapW, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFbo);
    glBindTexture(GL_TEXTURE_2D, depthMapBlur);
    glUniform2f(blurProgramScaleLoc, 0, 2.0f / kShadowMapH);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glEnable(GL_DEPTH_TEST);
}

void GLES3Renderer::initRenderModel(const RenderModel &model) {
    LOGV("Begin render model init");

    GLint uWorldMatrixLoc = -1;
    GLint uCameraMatrixLoc = -1;
    GLint uWorldMatrixPrevLoc = -1;
    GLint uCameraMatrixPrevLoc = -1;

    GLint aPosLoc = 0;
    GLint aNormLoc = 1;
    GLint aTexcoordLoc = 2;

    GLuint vbo, ibo;
    GLuint vao;
    GLuint shaderProgram;
    GLuint texture;

    {
        shaderProgram = shadowRenderProgram;
        shadowMapsEnabled ? shadowRenderProgram : createDiffuseOnlyShaderProgram();

        uWorldMatrixLoc = glGetUniformLocation(shaderProgram, "worldmatrix");
        uCameraMatrixLoc = glGetUniformLocation(shaderProgram, "projmatrix");
        lastCameraProjLoc = glGetUniformLocation(shaderProgram, "projmatrixPrev");
        uWorldMatrixPrevLoc = glGetUniformLocation(shaderProgram, "worldmatrixPrev");
    }

    {
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ibo);

        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
        glBufferData(GL_ARRAY_BUFFER,
                     model.geometry.vertexData.size() * sizeof(OBJParse::VertexAttributes),
                     &model.geometry.vertexData[0], GL_STATIC_DRAW);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     model.geometry.indexData.size() * sizeof(uint32_t),
                     &model.geometry.indexData[0], GL_STATIC_DRAW);

        glEnableVertexAttribArray(aPosLoc);
        glEnableVertexAttribArray(aNormLoc);
        glEnableVertexAttribArray(aTexcoordLoc);
        glVertexAttribPointer(aPosLoc, 3, GL_FLOAT, GL_FALSE, sizeof(OBJParse::VertexAttributes),
                              0);
        glVertexAttribPointer(aNormLoc, 3, GL_FLOAT, GL_FALSE, sizeof(OBJParse::VertexAttributes),
                              (void *) (uintptr_t) (3 * sizeof(GLfloat)));
        glVertexAttribPointer(aTexcoordLoc, 2, GL_FLOAT, GL_FALSE,
                              sizeof(OBJParse::VertexAttributes),
                              (void *) (uintptr_t) (6 * sizeof(GLfloat)));

        glBindVertexArray(defaultVao);

        glDisableVertexAttribArray(aPosLoc);
        glDisableVertexAttribArray(aNormLoc);
        glDisableVertexAttribArray(aTexcoordLoc);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }

    // init texture
    {
        glGenTextures(1, &texture);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, model.diffuseTexWidth, model.diffuseTexHeight, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, &model.diffuseRGBA8[0]);
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

        glBindTexture(GL_TEXTURE_2D, 0);
    }

    renderStates.push_back({
                                   shaderProgram,
                                   uWorldMatrixLoc,
                                   uCameraMatrixLoc,
                                   aPosLoc,
                                   aNormLoc,
                                   aTexcoordLoc,
                                   vbo,
                                   ibo,
                                   texture,
                                   0 /* texture for GL_TEXTURE1 */,
                                   vao,
                                   uWorldMatrixPrevLoc,
                                   uCameraMatrixPrevLoc});
}

void GLES3Renderer::preDrawUpdate() {

    lastCameraMatrix = currentCameraMatrix;
    const WorldState::CameraInfo &caminfo =
            world->cameraInfos[world->currentCamera];
    world->entities[world->currentCamera].updateCameraMatrix(
            caminfo.fov, caminfo.aspect,
            caminfo.near, caminfo.far,
            caminfo.right, caminfo.isOrtho,
            currentCameraMatrix);

    if (hasSkybox) {
        lastCameraSkyboxMatrix = currentCameraSkyboxMatrix;
        world->entities[world->currentCamera].updateCameraSkyboxMatrix(
                caminfo.fov, caminfo.aspect,
                caminfo.near, caminfo.far,
                caminfo.right, caminfo.isOrtho,
                currentCameraSkyboxMatrix);
    }

    if (shadowMapsEnabled) {
        const WorldState::CameraInfo &caminfo =
                world->cameraInfos[world->currentLight];
        world->entities[world->currentLight].updateCameraMatrix(
                caminfo.fov, caminfo.aspect,
                caminfo.near, caminfo.far,
                caminfo.right, caminfo.isOrtho,
                currentLightMatrix);
        shadowLightPos_x = world->entities[world->currentLight].pos.x;
        shadowLightPos_y = world->entities[world->currentLight].pos.y;
        shadowLightPos_z = world->entities[world->currentLight].pos.z;
    }

    objects.resize(world->entities.size());

    uint32_t oi = 0;
    for (const auto &ent : world->entities) {
        objects[oi].lastWorldMatrix = objects[oi].worldMatrix;
        objects[oi].visible = ent.renderable;
        objects[oi].renderHandle = ent.renderModel;
        objects[oi].indexCount =
                (uint32_t) world->renderModels[ent.renderModel].geometry.indexData.size();
        ent.updateWorldMatrix(objects[oi].worldMatrix);
        oi++;
    }

    lastCameraPos = world->entities[world->currentCamera].pos;
}

void GLES3Renderer::initializeRenderState(
        const GLES3Renderer::RenderState &targetState, bool forDepth) {

    if (!forDepth) glUseProgram(targetState.shaderProgram);

    glBindVertexArray(targetState.vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, targetState.ibo);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, targetState.texture0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, targetState.texture1);

    glActiveTexture(GL_TEXTURE0);
}

void GLES3Renderer::changeRenderState(render_state_handle_t handle, bool forDepth) {
    const GLES3Renderer::RenderState &targetState =
            renderStates[handle];

    if (!renderStateInitialized) {
        currRenderState = targetState;
        initializeRenderState(currRenderState, forDepth);
        renderStateInitialized = true;
    } else {

        if (!forDepth &&
            currRenderState.shaderProgram != targetState.shaderProgram) {
            glUseProgram(targetState.shaderProgram);
            currRenderState.shaderProgram = targetState.shaderProgram;
        }

        if (currRenderState.vbo != targetState.vbo) {
            glBindVertexArray(targetState.vao);
            currRenderState.vbo = targetState.vbo;
            currRenderState.aPosLoc = targetState.aPosLoc;
            currRenderState.aNormLoc = targetState.aNormLoc;
            currRenderState.aTexcoordLoc = targetState.aTexcoordLoc;
        }

        if (currRenderState.ibo != targetState.ibo) {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, targetState.ibo);
            currRenderState.ibo = targetState.ibo;
        }

        if (currRenderState.texture0 != targetState.texture0) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, targetState.texture0);
            currRenderState.texture0 = targetState.texture0;
        }

        if (currRenderState.texture1 != targetState.texture1) {
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, targetState.texture1);
            currRenderState.texture1 = targetState.texture1;
        }

        if (!forDepth) {
            currRenderState.uWorldMatrixLoc = targetState.uWorldMatrixLoc;
            currRenderState.uCameraMatrixLoc = targetState.uCameraMatrixLoc;
        }

        glActiveTexture(GL_TEXTURE0);
    }
}

void GLES3Renderer::finalPass() {
    glDisable(GL_DEPTH_TEST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, windowWidth, windowHeight);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(finalPassProgram);
    glUniform1i(finalPassProgramColorSamplerLoc, 0);
    glUniform1i(finalPassProgramVelocitySamplerLoc, 1);

    glBindVertexArray(defaultVao);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, lastSceneFboColor0Texture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, lastSceneFboColor1Velocity);
    glActiveTexture(GL_TEXTURE0);

    glDrawArrays(GL_TRIANGLES, 0, 6);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);
    glEnable(GL_DEPTH_TEST);
}

void GLES3Renderer::draw() {
    render_state_handle_t lastRenderState = -1;

    if (shadowMapsEnabled) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, 0);

        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFbo);
        glViewport(0, 0, kShadowMapW, kShadowMapH);
        glClear(GL_DEPTH_BUFFER_BIT);
        glUseProgram(depthMapProgram);


        {
            for (const auto &obj: objects) {
                if (!(obj.visible)) continue;
                changeRenderState(obj.renderHandle, true);
                glUniformMatrix4fv(depthMapWorldMatrixLoc,
                                   1, GL_FALSE, (currentLightMatrix * obj.worldMatrix).vals);
                glDrawElements(GL_TRIANGLES, obj.indexCount, GL_UNSIGNED_INT, 0);
            }
        }

        blurPass();

        glBindFramebuffer(GL_FRAMEBUFFER, lastSceneFbo);
        glViewport(0, 0, windowWidth, windowHeight);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        GLint shadowRenderDiffuseLoc =
                glGetUniformLocation(shadowRenderProgram, "diffuse");
        GLint shadowRenderDepthMapSamplerLoc =
                glGetUniformLocation(shadowRenderProgram, "depthMapFromLight");
        GLint shadowRenderLightMatrixLoc =
                glGetUniformLocation(shadowRenderProgram, "projMatrixLight");

        glUseProgram(shadowRenderProgram);
        glUniform1i(shadowRenderDiffuseLoc, 0);
        glUniform1i(shadowRenderDepthMapSamplerLoc, 1);
        glUniform3f(shadowLightPosUniformLoc,
                    shadowLightPos_x,
                    shadowLightPos_y,
                    shadowLightPos_z);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, depthMapDestination);
        glActiveTexture(GL_TEXTURE0);

        {
            for (const auto &obj: objects) {
                if (!(obj.visible)) continue;
                changeRenderState(obj.renderHandle);
                glUniformMatrix4fv(currRenderState.uWorldMatrixLoc,
                                   1, GL_FALSE, (obj.worldMatrix).vals);
                glUniformMatrix4fv(currRenderState.uCameraMatrixLoc,
                                   1, GL_FALSE, currentCameraMatrix.vals);
                glUniformMatrix4fv(shadowRenderLightMatrixLoc,
                                   1, GL_FALSE, currentLightMatrix.vals);
                glUniformMatrix4fv(lastCameraProjLoc,
                                   1, GL_FALSE, (lastCameraMatrix.vals));
                glUniformMatrix4fv(currRenderState.uWorldMatrixPrevLoc,
                                   1, GL_FALSE, (obj.lastWorldMatrix).vals);
                glDrawElements(GL_TRIANGLES, obj.indexCount, GL_UNSIGNED_INT, 0);
            }
        }

        if (hasSkybox) {
            renderSkybox();
        }

        finalPass();
    } else {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        for (const auto &obj: objects) {
            if (!(obj.visible)) continue;
            changeRenderState(obj.renderHandle);
            glUniformMatrix4fv(currRenderState.uWorldMatrixLoc,
                               1, GL_FALSE, (obj.worldMatrix).vals);
            glUniformMatrix4fv(currRenderState.uCameraMatrixLoc,
                               1, GL_FALSE, currentCameraMatrix.vals);
            glDrawElements(GL_TRIANGLES, obj.indexCount, GL_UNSIGNED_INT, 0);
        }
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glUseProgram(0);
    glBindTexture(GL_TEXTURE_2D, 0);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);

    renderStateInitialized = false;
}

static const float sSkyboxPositions[] = {
        -1.0f, 1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f, 1.0f, -1.0f,
        -1.0f, 1.0f, -1.0f,

        -1.0f, -1.0f, 1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f, 1.0f, -1.0f,
        -1.0f, 1.0f, -1.0f,
        -1.0f, 1.0f, 1.0f,
        -1.0f, -1.0f, 1.0f,

        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f, 1.0f,
        -1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        1.0f, -1.0f, 1.0f,
        -1.0f, -1.0f, 1.0f,

        -1.0f, 1.0f, -1.0f,
        1.0f, 1.0f, -1.0f,
        1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        -1.0f, 1.0f, 1.0f,
        -1.0f, 1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f, 1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f, 1.0f,
        1.0f, -1.0f, 1.0f
};

void GLES3Renderer::initSkybox() {
    LOGV("%s: call", __func__);

    glGenVertexArrays(1, &skyboxVao);
    glBindVertexArray(skyboxVao);

    glGenBuffers(1, &skyboxVbo);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(sSkyboxPositions), sSkyboxPositions, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid *) 0);
    glBindVertexArray(defaultVao);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    skyboxProgram = compileShaderProgram(
            sSkyboxVShaderSrc,
            sSkyboxFShaderSrc,
            nullptr);

    skyboxSamplerUniformLoc =
            glGetUniformLocation(skyboxProgram, "skyboxTexture");

    skyboxMatrixUniformLoc =
            glGetUniformLocation(skyboxProgram, "viewproj");

    LOGV("%s: locs %d %d", __func__,
         skyboxSamplerUniformLoc,
         skyboxMatrixUniformLoc);

    glGenTextures(1, &skyboxTexture);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexture);

    for (GLuint i = 0; i < 6; i++) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                     0, GL_RGBA,
                     world->skyboxTexWidth, world->skyboxTexHeight, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE,
                     &world->skyboxData[i][0]);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

void GLES3Renderer::renderSkybox() {
    glActiveTexture(GL_TEXTURE0);

    glUseProgram(skyboxProgram);
    glUniform1i(skyboxSamplerUniformLoc, 0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexture);
    glBindVertexArray(skyboxVao);
    glUniformMatrix4fv(skyboxMatrixUniformLoc, 1, GL_FALSE,
                       currentCameraSkyboxMatrix.vals);
    glDepthFunc(GL_LEQUAL);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glDepthFunc(GL_LESS);

    glBindVertexArray(defaultVao);
    glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexture);
}
