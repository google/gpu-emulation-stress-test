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

static const char *const sDiffuseOnlyVShaderSrc = R"(#version 330 core
uniform mat4 worldmatrix;
uniform mat4 projmatrix;
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 v3NormalIn;
layout (location = 2) in vec2 v2TexCoordsIn;
out vec2 v2TexCoord;
void main() {
	v2TexCoord = v2TexCoordsIn;
	gl_Position = projmatrix * worldmatrix * vec4(position.xyz, 1);
})";

static const char *const sDiffuseOnlyFShaderSrc = R"(#version 330 core
uniform sampler2D diffuse;
in vec2 v2TexCoord;
out fragColor;
void main() {
    fragColor = texture(diffuse, v2TexCoord);
})";

// Shadow mapping
static const char *const sDepthMapVShaderSrc = R"(#version 330 core
uniform mat4 worldmatrix;
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 v3NormalIn;
layout (location = 2) in vec2 v2TexCoordsIn;
out vec4 coordVarying;
void main() {
    coordVarying = worldmatrix * vec4(position.xyz, 1);
    gl_Position = coordVarying;
})";

static const char *const sDepthMapFShaderSrc = R"(#version 330 core
in vec4 coordVarying;
out vec4 fragColor;
void main() {
    vec3 divPos = coordVarying.xyz / coordVarying.w;
    float depth = divPos.z;
    depth = depth * 0.5 + 0.5;
    fragColor = vec4(depth, 0.0, 0.0, 0.0);
})";

// Blur part of shadow mapping
static const char *const sBlurVShaderSrc = R"(#version 330 core
out vec2 v_texcoord;
void main() {
    const vec2 quad_pos[6] = vec2[6](
        vec2(0.0, 0.0),
        vec2(0.0, 1.0),
        vec2(1.0, 0.0),
        vec2(0.0, 1.0),
        vec2(1.0, 0.0),
        vec2(1.0, 1.0));
    gl_Position = vec4((quad_pos[gl_VertexID] * 2.0) - 1.0, 0.0, 1.0);
    v_texcoord = quad_pos[gl_VertexID];
})";

static const char *const sBlurFShaderSrc = R"(#version 330 core
in vec2 v_texcoord;

uniform sampler2D toBlur;
uniform vec2 scale;
out vec4 outColor;

void main() {
   const float weights[7] = float[7](
       0.006, 0.061, 0.242, 0.383, 0.242, 0.061, 0.006);
	vec4 color = vec4(0.0);
    for (int i = -3; i < 4; i++) {
        color += weights[i + 3] * (texture(toBlur, v_texcoord.xy + float(i) * scale));
    }
    outColor = vec4(color.rgb, 1.0);
})";

static const char *const sShadowRenderVShaderSrc = R"(#version 330 core
uniform mat4 worldmatrix;
uniform mat4 projmatrix;
uniform mat4 projMatrixLight;

uniform mat4 worldmatrixPrev;

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 v3NormalIn;
layout (location = 2) in vec2 v2TexCoordsIn;

out vec2 v2TexCoord;

out vec4 prevPos;
out vec4 fragPos;
out vec3 fragNorm;

out vec4 depthMapCoord;

void main() {
    v2TexCoord = v2TexCoordsIn;

    vec4 worldPos = worldmatrix * vec4(position.xyz, 1);
    prevPos = worldmatrixPrev * vec4(position.xyz, 1);
    fragPos = worldPos;
    fragNorm = (worldmatrix * vec4(v3NormalIn, 1)).xyz;

    depthMapCoord = projMatrixLight * worldPos;
	gl_Position = projmatrix * worldPos;
})";

static const int kShadowMapW = 2048;
static const int kShadowMapH = 2048;
/*static const char* const sShadowRenderFShaderSrc = R"(#version 330 core

uniform sampler2D diffuse;
uniform sampler2DShadow depthMapFromLight;

uniform vec3 lightPos;

in vec2 v2TexCoord;

in vec3 fragPos;
in vec3 fragNorm;

in vec4 depthMapCoord;

in vec4 prevPos;

void main() {

    vec3 depthMapPos = (depthMapCoord / depthMapCoord.w).xyz;
    depthMapPos = depthMapPos * 0.5 + 0.5;
    depthMapPos.z = clamp(depthMapPos.z - 0.0006, 0.0, 1.0);

    vec3 normalizedNormal = normalize(fragNorm); // heh
    float nDotL = dot(normalizedNormal, normalize(lightPos - fragPos));
    float diffuseAmbientAttenuation = 0.7 + 0.3 * nDotL;

    float shadowAcc = 0.0;
    shadowAcc = texture(depthMapFromLight, depthMapPos);

    float shadowAttenuation = 0.7 + 0.3 * (1.0 - shadowAcc);

    float overbright = 1.3;
    fragColor =
        overbright *
        shadowAttenuation *
        diffuseAmbientAttenuation *
        texture(diffuse, v2TexCoord);
})";*/

static const char *const sShadowRenderFShaderESMSrc = R"(#version 330 core

uniform sampler2D diffuse;
uniform sampler2D depthMapFromLight;

uniform vec3 lightPos;
uniform mat4 projmatrix;
uniform mat4 projmatrixPrev;

in vec2 v2TexCoord;

in vec4 fragPos;
in vec4 prevPos;
in vec3 fragNorm;

in vec4 depthMapCoord;

out vec4 fragData[2];

float shadowAttenuation(vec3 depthCoord) {
    float sceneDepth = depthCoord.z;
    float occlude = texture(depthMapFromLight, depthCoord.xy).r;
    return clamp(exp(144.0 * (occlude - sceneDepth)), 0.0, 1.0);
}

void main() {

    vec3 depthMapPos = (depthMapCoord / depthMapCoord.w).xyz;
    depthMapPos = depthMapPos * 0.5 + 0.5;

    vec3 normalizedNormal = normalize(fragNorm); // heh
    float nDotL = dot(normalizedNormal, normalize(lightPos - fragPos.xyz));
    float diffuseAmbientAttenuation = 0.7 + 0.3 * nDotL;

    float shadowAttenuation =
        0.5 + 0.5 * shadowAttenuation(depthMapPos);

    float overbright = 1.3;

    vec4 sceneColor =
        overbright *
        shadowAttenuation *
        diffuseAmbientAttenuation *
        texture(diffuse, v2TexCoord);

    vec4 ndcCurrPos = projmatrix * fragPos;
    ndcCurrPos = ndcCurrPos / ndcCurrPos.w;

    vec4 ndcPrevPos = projmatrixPrev * prevPos;
    ndcPrevPos = ndcPrevPos / ndcPrevPos.w;

    vec2 velocity = ndcCurrPos.xy - ndcPrevPos.xy;
    velocity *= vec2(1920.0, 1080.0);
    velocity = velocity * 0.125 + 0.5;
    vec4 velocityColor = vec4(velocity.xy, 0.0, 0.0);

    fragData[0] = sceneColor;
    fragData[1] = velocityColor;
})";

static const char *const sFinalPassVShaderSrc = R"(#version 330 core
out vec2 v_texcoord;
void main() {
    const vec2 quad_pos[6] = vec2[6](
        vec2(0.0, 0.0),
        vec2(0.0, 1.0),
        vec2(1.0, 0.0),
        vec2(0.0, 1.0),
        vec2(1.0, 0.0),
        vec2(1.0, 1.0));
    gl_Position = vec4((quad_pos[gl_VertexID] * 2.0) - 1.0, 0.0, 1.0);
    v_texcoord = quad_pos[gl_VertexID];
})";

static const char *const sFinalPassFShaderSrc = R"(#version 330 core

uniform sampler2D color;
uniform sampler2D velocity;

in vec2 v_texcoord;
out vec4 finalColor;
void main() {

    vec4 result = vec4(0.0);
    vec2 rawVelocity = texture(velocity, v_texcoord).xy;
    vec2 velocity = 4.0 * (rawVelocity - 0.5);
    velocity /= vec2(1920.0, 1080.0);

    for (int i = -5; i < 5; i++) {
        result += 0.10 * texture(color, v_texcoord + 0.20 * float(i) * velocity);
    }

    finalColor = result;
})";

// Skybox
static const char *const sSkyboxVShaderSrc = R"(#version 330 core
layout (location = 0) in vec3 pos;
out vec3 texCoord;

uniform mat4 viewproj;

void main() {
    texCoord = pos;
    vec4 pos = viewproj * vec4(pos, 1.0);
    gl_Position = pos.xyww;
})";

static const char *const sSkyboxFShaderSrc = R"(#version 330 core
uniform samplerCube skyboxTexture;
in vec3 texCoord;
out vec4 fragData[2];

void main() {
    fragData[0] = texture(skyboxTexture, texCoord.xzy);
    fragData[1] = vec4(0.5, 0.5, 0.0, 0.0);
})";


