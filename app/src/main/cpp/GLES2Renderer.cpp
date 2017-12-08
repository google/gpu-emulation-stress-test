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

#include "GLES2Renderer.h"
#include "ScopedProfiler.h"

#include "log.h"

void GLES2Renderer::reInit(WorldState *worldState, int width, int height) {

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

    // init skybox
}

static const char *const sDiffuseOnlyVShaderSrc = R"(
uniform highp mat4 worldmatrix;
uniform highp mat4 projmatrix;
attribute highp vec3 position;
attribute highp vec3 v3NormalIn;
attribute highp vec2 v2TexCoordsIn;
varying highp vec2 v2TexCoord;
void main() {
	v2TexCoord = v2TexCoordsIn;
	gl_Position = projmatrix * worldmatrix * vec4(position.xyz, 1);
})";

static const char *const sDiffuseOnlyFShaderSrc = R"(
uniform sampler2D diffuse;
varying highp vec2 v2TexCoord;
void main() {
    gl_FragColor = texture2D(diffuse, v2TexCoord);
})";

// Shadow mapping
static const char *const sDepthMapVShaderSrc = R"(
uniform highp mat4 worldmatrix;
attribute highp vec3 position;
attribute highp vec3 v3NormalIn;
attribute highp vec2 v2TexCoordsIn;
void main() {
    gl_Position = worldmatrix * vec4(position.xyz, 1);
})";

static const char *const sDepthMapFShaderSrc = R"(
void main() {
    // Just let depth values get written out
})";

static const char *const sShadowRenderVShaderSrc = R"(
uniform highp mat4 worldmatrix;
uniform highp mat4 projmatrix;
uniform highp mat4 projMatrixLight;

attribute highp vec3 position;
attribute highp vec3 v3NormalIn;
attribute highp vec2 v2TexCoordsIn;

varying highp vec2 v2TexCoord;

varying highp vec3 fragPos;
varying highp vec3 fragNorm;

varying highp vec4 depthMapCoord;

void main() {
    v2TexCoord = v2TexCoordsIn;

    vec4 worldPos = worldmatrix * vec4(position.xyz, 1);

    fragPos = worldPos.xyz;
    fragNorm = (worldmatrix * vec4(v3NormalIn, 1)).xyz;

    depthMapCoord = projMatrixLight * worldPos;
	gl_Position = projmatrix * worldPos;
}
)";

static const int kShadowMapW = 4096;
static const int kShadowMapH = 4096;
static const char *const sShadowRenderFShaderSrc = R"(
precision highp float;

uniform sampler2D diffuse;
uniform sampler2D depthMapFromLight;

uniform vec3 lightPos;

varying highp vec2 v2TexCoord;

varying highp vec3 fragPos;
varying highp vec3 fragNorm;

varying highp vec4 depthMapCoord;

void main() {
    vec4 depthMapPos = depthMapCoord / depthMapCoord.w;
    depthMapPos = depthMapPos * 0.5 + 0.5;
    float lightDepth = depthMapPos.z;

    vec3 normalizedNormal = normalize(fragNorm); // heh
    float nDotL = dot(normalizedNormal, normalize(lightPos - fragPos));
    float diffuseAmbientAttenuation =
        0.7 + 0.3 * nDotL;

    float shadowAcc = 0.0;
    float texelSizeInv = 1.0 / 4096.0;
    float texelSize = 4096.0;

    //float cosTheta = clamp( nDotL, 0.0, 1.0 );
    //float bias = 0.0002*tan(acos(cosTheta));
    //bias = clamp(bias, 0.0, 0.0003);

    //float bias = max(0.003* (1.0 - nDotL), 0.003);
    float biasedDepth = lightDepth - 0.0006;

    // linearly filtered 5x5 pcf

    int x = 0; int y = 0;

    vec2 thisUV = depthMapPos.xy + vec2(x, y) * texelSizeInv;
    vec2 f = fract(thisUV * texelSize + 0.5);
    vec2 centroidUV = floor(thisUV * texelSize + 0.5) / texelSize;

    vec2 downl = centroidUV;
    vec2 upl =   centroidUV + vec2(0.0, 1.0) * texelSizeInv;
    vec2 downr = centroidUV + vec2(1.0, 0.0) * texelSizeInv;
    vec2 upr =   centroidUV + vec2(1.0, 1.0) * texelSizeInv;

    float acc_downl = step(texture2D(depthMapFromLight, downl).x, biasedDepth);
    float acc_upl = step(texture2D(depthMapFromLight, upl).x, biasedDepth);
    float acc_downr = step(texture2D(depthMapFromLight, downr).x, biasedDepth);
    float acc_upr = step(texture2D(depthMapFromLight, upr).x, biasedDepth);
            
    float res_l = mix(acc_downl, acc_upl, f.y);
    float res_r = mix(acc_downr, acc_upr, f.y);
    float res = mix(res_l, res_r, f.x);

    shadowAcc += res;

    float shadowAttenuation = 0.7 + 0.3 * (1.0 - shadowAcc);

    float overbright = 1.3;
    gl_FragColor =
        overbright *
        shadowAttenuation *
        diffuseAmbientAttenuation *
        texture2D(diffuse, v2TexCoord);
})";

// Skybox
static const char *const sSkyboxVShaderSrc = R"(
attribute highp vec3 pos;
varying highp vec3 texCoord;

uniform highp mat4 viewproj;

void main() {
    texCoord = pos;
    vec4 pos = viewproj * vec4(pos, 1.0);
    gl_Position = pos.xyww;
})";

static const char *const sSkyboxFShaderSrc = R"(
uniform samplerCube skyboxTexture;
varying highp vec3 texCoord;

void main() {
    gl_FragColor = textureCube(skyboxTexture, texCoord.xzy);
})";

GLuint GLES2Renderer::compileAndValidateShader(GLenum shaderType, const char *src) {
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
        LOGE("%s: fail to compile. infolog: %s", __FUNCTION__,
             &infologBuf[0]);
    }

    return shader;
}

GLuint GLES2Renderer::compileShaderProgram(const char *vshaderSrc, const char *fshaderSrc,
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
        LOGE("%s: fail to link. infolog: %s", __FUNCTION__,
             &infologBuf[0]);
    }

    return program;
}

static void bindVertexAttribLocsModel(GLuint prog) {
    glBindAttribLocation(prog, 0, "position");
    glBindAttribLocation(prog, 1, "v3NormalIn");
    glBindAttribLocation(prog, 2, "v2TexCoordsIn");
}

GLuint GLES2Renderer::createDiffuseOnlyShaderProgram() {
    return compileShaderProgram(
            sDiffuseOnlyVShaderSrc,
            sDiffuseOnlyFShaderSrc,
            bindVertexAttribLocsModel);
}

void GLES2Renderer::initShadowRendererState() {

    LOGD("%s: compile depth map prog", __FUNCTION__);
    depthMapProgram =
            compileShaderProgram(
                    sDepthMapVShaderSrc,
                    sDepthMapFShaderSrc,
                    bindVertexAttribLocsModel);

    LOGD("%s: compile shadow render prog", __FUNCTION__);
    shadowRenderProgram =
            compileShaderProgram(
                    sShadowRenderVShaderSrc,
                    sShadowRenderFShaderSrc,
                    bindVertexAttribLocsModel);

    shadowLightPosUniformLoc =
            glGetUniformLocation(shadowRenderProgram, "lightPos");

    LOGD("%s: init shadow map fbo", __FUNCTION__);

    { // create depth texture
        glGenTextures(1, &depthMapTexture);
        glBindTexture(GL_TEXTURE_2D, depthMapTexture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0,
                     GL_DEPTH_COMPONENT, kShadowMapW, kShadowMapH, 0,
                     GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, 0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    { // create depth map FBO
        glGenFramebuffers(1, &depthMapFbo);
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMapTexture,
                               0);

        int fboCompleteness = glCheckFramebufferStatus(GL_FRAMEBUFFER);

        if (fboCompleteness != GL_FRAMEBUFFER_COMPLETE) {
            LOGE("%s: depth map fbo not complete :( 0x%x", __FUNCTION__, fboCompleteness);
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
}

void GLES2Renderer::initRenderModel(const RenderModel &model) {
    LOGV("Begin render model init");
    // init shader program

    GLint uWorldMatrixLoc = -1;
    GLint uCameraMatrixLoc = -1;

    GLuint shaderProgram = shadowMapsEnabled ? shadowRenderProgram
                                             : createDiffuseOnlyShaderProgram();

    uWorldMatrixLoc = glGetUniformLocation(shaderProgram, "worldmatrix");
    uCameraMatrixLoc = glGetUniformLocation(shaderProgram, "projmatrix");
    GLint aPosLoc = 0;
    GLint aNormLoc = 1;
    GLint aTexcoordLoc = 2;

    // init vbo, ibo

    GLuint vbo, ibo;

    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ibo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    // LOGD("New vbo %u vertex bytes %d", vbo, model.geometry.vertexData.size() * sizeof(OBJParse::VertexAttributes));
    glBufferData(GL_ARRAY_BUFFER,
                 model.geometry.vertexData.size() * sizeof(OBJParse::VertexAttributes),
                 &model.geometry.vertexData[0], GL_STATIC_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 model.geometry.indexData.size() * sizeof(uint32_t),
                 &model.geometry.indexData[0], GL_STATIC_DRAW);

    glEnableVertexAttribArray(aPosLoc);
    glEnableVertexAttribArray(aNormLoc);
    glEnableVertexAttribArray(aTexcoordLoc);
    glVertexAttribPointer(aPosLoc, 3, GL_FLOAT, GL_FALSE, sizeof(OBJParse::VertexAttributes), 0);
    glVertexAttribPointer(aNormLoc, 3, GL_FLOAT, GL_FALSE, sizeof(OBJParse::VertexAttributes),
                          (void *) (uintptr_t) (3 * sizeof(GLfloat)));
    glVertexAttribPointer(aTexcoordLoc, 2, GL_FLOAT, GL_FALSE, sizeof(OBJParse::VertexAttributes),
                          (void *) (uintptr_t) (6 * sizeof(GLfloat)));

    glDisableVertexAttribArray(aPosLoc);
    glDisableVertexAttribArray(aNormLoc);
    glDisableVertexAttribArray(aTexcoordLoc);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // init texture

    GLuint texture;
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
                                   0, 0, 0 /* VAO, prev world and camera matrices */});
}

void GLES2Renderer::preDrawUpdate() {
    //ScopedProfiler updateProfile("preDrawUpdateProfile");

    const WorldState::CameraInfo &caminfo =
            world->cameraInfos[world->currentCamera];
    world->entities[world->currentCamera].updateCameraMatrix(
            caminfo.fov, caminfo.aspect,
            caminfo.near, caminfo.far,
            caminfo.right, caminfo.isOrtho,
            currentCameraMatrix);

    if (hasSkybox) {
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
        objects[oi].visible = ent.renderable;
        objects[oi].renderHandle = ent.renderModel;
        objects[oi].indexCount = (uint32_t) world->renderModels[ent.renderModel].geometry.indexData.size();
        ent.updateWorldMatrix(objects[oi].worldMatrix);
        oi++;
    }

}

void
GLES2Renderer::initializeRenderState(const GLES2Renderer::RenderState &targetState, bool forDepth) {
    if (!forDepth) glUseProgram(targetState.shaderProgram);

    updateVertexAttributes(targetState);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, targetState.ibo);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, targetState.texture0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, targetState.texture1);

    glActiveTexture(GL_TEXTURE0);
}

void GLES2Renderer::updateVertexAttributes(const GLES2Renderer::RenderState &targetState) {
    glBindBuffer(GL_ARRAY_BUFFER, targetState.vbo);
    glVertexAttribPointer(targetState.aPosLoc, 3, GL_FLOAT, GL_FALSE,
                          sizeof(OBJParse::VertexAttributes), 0);
    glVertexAttribPointer(targetState.aNormLoc, 3, GL_FLOAT, GL_FALSE,
                          sizeof(OBJParse::VertexAttributes),
                          (void *) (uintptr_t) (3 * sizeof(GLfloat)));
    glVertexAttribPointer(targetState.aTexcoordLoc, 2, GL_FLOAT, GL_FALSE,
                          sizeof(OBJParse::VertexAttributes),
                          (void *) (uintptr_t) (6 * sizeof(GLfloat)));
}

void GLES2Renderer::changeRenderState(render_state_handle_t handle, bool forDepth) {
    const GLES2Renderer::RenderState &targetState =
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

        // vbo is used as the key for our fake vao
        if (currRenderState.vbo != targetState.vbo) {
            updateVertexAttributes(targetState);
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

void GLES2Renderer::draw() {
    render_state_handle_t lastRenderState = -1;
    setModelVertexAttribs();

    if (shadowMapsEnabled) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, 0);

        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFbo);
        glViewport(0, 0, kShadowMapW, kShadowMapH);
        glClear(GL_DEPTH_BUFFER_BIT);
        glUseProgram(depthMapProgram);

        GLint depthMapWorldMatrixLoc =
                glGetUniformLocation(depthMapProgram, "worldmatrix");

        {
            // ScopedProfiler updateProfile("shadowDraw");
            for (const auto &obj: objects) {
                if (!(obj.visible)) continue;
                changeRenderState(obj.renderHandle, true);
                glUniformMatrix4fv(depthMapWorldMatrixLoc,
                                   1, GL_FALSE, (currentLightMatrix * obj.worldMatrix).vals);
                glDrawElements(GL_TRIANGLES, obj.indexCount, GL_UNSIGNED_INT, 0);
            }
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
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
        glBindTexture(GL_TEXTURE_2D, depthMapTexture);
        glActiveTexture(GL_TEXTURE0);

        {
            // ScopedProfiler updateProfile("litDraw");
            for (const auto &obj: objects) {
                if (!(obj.visible)) continue;
                changeRenderState(obj.renderHandle);
                glUniformMatrix4fv(currRenderState.uWorldMatrixLoc,
                                   1, GL_FALSE, (obj.worldMatrix).vals);
                glUniformMatrix4fv(currRenderState.uCameraMatrixLoc,
                                   1, GL_FALSE, currentCameraMatrix.vals);
                glUniformMatrix4fv(shadowRenderLightMatrixLoc,
                                   1, GL_FALSE, currentLightMatrix.vals);
                glDrawElements(GL_TRIANGLES, obj.indexCount, GL_UNSIGNED_INT, 0);
            }
        }

        if (hasSkybox) {
            renderSkybox();
        }
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

static void bindVertexAttribLocsSkybox(GLuint prog) {
    glBindAttribLocation(prog, 0, "pos");
}

void GLES2Renderer::initSkybox() {
    LOGD("%s: call", __func__);

    glGenBuffers(1, &skyboxVbo);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(sSkyboxPositions), sSkyboxPositions, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    skyboxProgram = compileShaderProgram(
            sSkyboxVShaderSrc,
            sSkyboxFShaderSrc,
            bindVertexAttribLocsSkybox);

    skyboxSamplerUniformLoc =
            glGetUniformLocation(skyboxProgram, "skyboxTexture");

    skyboxMatrixUniformLoc =
            glGetUniformLocation(skyboxProgram, "viewproj");

    LOGD("%s: locs %d %d", __func__,
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
// 
//     // Skybox render state
//     GLuint skyboxProgram;
//     GLuint skyboxTexture;
//     GLint skyboxAttribLoc;
//     GLint skyboxMatrixUniformLoc;
//     GLint skyboxSamplerUniformLoc;
//     GLuint skyboxVbo;
}

void GLES2Renderer::renderSkybox() {
    glActiveTexture(GL_TEXTURE0);

    glUseProgram(skyboxProgram);
    glUniform1i(skyboxSamplerUniformLoc, 0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexture);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVbo);
    setSkyboxVertexAttribs();

    glUniformMatrix4fv(skyboxMatrixUniformLoc, 1, GL_FALSE,
                       currentCameraSkyboxMatrix.vals);

    glDepthFunc(GL_LEQUAL);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glDepthFunc(GL_LESS);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexture);
}

void GLES2Renderer::setModelVertexAttribs() {
    for (int i = 0; i < 16; i++) {
        glDisableVertexAttribArray(i);
    }

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(OBJParse::VertexAttributes), 0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(OBJParse::VertexAttributes),
                          (void *) (uintptr_t) (3 * sizeof(GLfloat)));
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(OBJParse::VertexAttributes),
                          (void *) (uintptr_t) (6 * sizeof(GLfloat)));

}

void GLES2Renderer::setSkyboxVertexAttribs() {
    for (int i = 0; i < 16; i++) {
        glDisableVertexAttribArray(i);
    }
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid *) 0);
}
