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

#include "WorldState.h"

#include "FileLoader.h"
#include "ScopedProfiler.h"
#include "TextureLoader.h"
#include "util.h"

#include <algorithm>

#define MAX_NAME_LEN 128

static char name1[MAX_NAME_LEN] = {};
static char name2[MAX_NAME_LEN] = {};
static char name3[MAX_NAME_LEN] = {};
static char name4[MAX_NAME_LEN] = {};
static char name5[MAX_NAME_LEN] = {};

static void resetNames() {
    memset(name1, 0, sizeof(name1));
    memset(name2, 0, sizeof(name2));
    memset(name3, 0, sizeof(name3));
    memset(name4, 0, sizeof(name4));
    memset(name5, 0, sizeof(name5));
}

void WorldState::loadFromFile(const std::string &filename, int numObjects) {
    std::vector<unsigned char> bytes =
            FileLoader::get()->loadFileFromAssets(filename);

    std::vector<std::string> lines =
            splitLines(std::string(bytes.begin(), bytes.end()));

    uint32_t framenum, handle;
    int i1, i2, i3, i4;
    float f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12;

    uint32_t gpuTextCount = 0;

    for (const auto &line : lines) {
        if (sscanf(&line[0], "define camera %s %u", name1, &handle) == 2) {
            defineCameraOrLight(name1, handle);
        } else if (sscanf(&line[0], "define light %s %u", name1, &handle) == 2) {
            defineCameraOrLight(name1, handle, true);
        } else if (sscanf(&line[0], "define model %s", name1) == 1) {
            defineModel(name1);
        } else if (sscanf(&line[0], "define entity %s %u", name1, &handle) == 2) {
            if (!strcmp("gpu_text", name1)) {
                gpuTextCount++;
            }
            defineEntity(name1, handle);
        } else if (sscanf(&line[0], "set %u proj %f %f %f %f",
                          &handle, &f1, &f2, &f3, &f4) == 5) {
            setProjectionMatrix(handle, f1, f2, f3, f4, 1.0f, false);
        } else if (sscanf(&line[0], "set %u orthoproj %f %f %f %f",
                          &handle, &f1, &f2, &f3, &f4) == 5) {
            setProjectionMatrix(handle, 0.0f, f1, f2, f3, f4, true);
        } else if (sscanf(&line[0], "set %u scale %f %f %f",
                          &handle, &f1, &f2, &f3) == 4) {
            setScale(handle, f1, f2, f3);
        } else if (sscanf(&line[0], "set %u frame %f %f %f %f %f %f %f %f %f",
                          &handle, &f1, &f2, &f3, &f4, &f5, &f6, &f7, &f8, &f9) == 10) {
            setWorldFrame(handle, f1, f2, f3, f4, f5, f6, f7, f8, f9);
        } else if (sscanf(&line[0], "set entityanim %u %u %f %f %f %f %f %f %f %f %f %f %f %f",
                          &framenum, &handle, &f1, &f2, &f3, &f4, &f5, &f6, &f7, &f8, &f9, &f10,
                          &f11, &f12) == 14) {
            setAnimFrame(framenum, handle, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12);
        } else if (sscanf(&line[0], "set %u prop int %s %d",
                          &handle, name1, &i1) == 3) {
            setIntProp(handle, name1, i1);
        } else if (sscanf(&line[0], "set %u prop float %s %f",
                          &handle, name1, &f1) == 3) {
            setFloatProp(handle, name1, f1);
        } else if (sscanf(&line[0], "set %u prop str %s %s",
                          &handle, name1, name2) == 3) {
            setStringProp(handle, name1, name2);
        } else if (sscanf(&line[0], "define curve %s", name1) == 1) {
            defineCurve(name1);
        } else if (sscanf(&line[0], "set curve %s %d %s %s %f %f %f %f %f %f %f %f %f",
                          name1, &i1, name2, name3, &f1, &f2, &f3, &f4, &f5, &f6, &f7, &f8, &f9) ==
                   13) {
            setCurve(name1, i1, name2, name3, f1, f2, f3, f4, f5, f6, f7, f8, f9);
        } else if (sscanf(&line[0], "set curveaction %s %s %s %s %s %f %f %f %f %f %f",
                          name1, name2, name3, name4, name5, &f1, &f2, &f3, &f4, &f5, &f6) == 11) {
            setCurveAction(name1, name2, name3, name4, name5, f1, f2, f3, f4, f5, f6);
        } else if (sscanf(&line[0], "define particles %s", name1) == 1) {
            defineParticles(name1);
        } else if (sscanf(&line[0], "set particles %s %d %d %d %d %f %f %s",
                          name1, &i1, &i2, &i3, &i4, &f1, &f2, name2) == 8) {
            // Override # particles
            setParticles(name1, numObjects, i2, i3, i4, f1, f2, name2);
        } else if (sscanf(&line[0], "set particlesmodel %s %s",
                          name1, name2) == 2) {
            setParticlesModel(name1, name2);
        }
        resetNames();
    }

    if (gpuTextCount != 1) {
        LOGE("Not genuine Android GPU Emulation Stress Test!");
        abort();
    }

    // initialize stuff
    // Curves
    for (auto it : curves) {
        it.second->precalcArclengths(16);
    }

    // Skybox
    skyboxName = "skybox_android";
    loadSkybox();
}

void WorldState::resetAspectRatio(int width, int height) {
    float aspect = (float) width / (float) height;
    for (auto &it : cameraInfos) {
        if (!it.isLight) {
            it.aspect = aspect;
        }
    }
}

void
WorldState::defineCameraOrLight(const std::string &name, entity_handle_t handle, bool isLight) {
    LOGV("name %s handle %u", name.c_str(), handle);
    addEntity(handle, name);
    addCameraInfo(handle, isLight);
    if (!isLight) {
        currentCamera = handle;
    } else {
        currentLight = handle;
    }
    entities[handle].renderable = false;
}

void WorldState::defineModel(const std::string &name) {
    LOGV("name %s", name.c_str());
    addRenderModel(name);
}

void WorldState::defineEntity(const std::string &name, entity_handle_t handle) {
    LOGV("name %s handle %u", name.c_str(), handle);
    addEntity(handle, name);

}

void WorldState::setProjectionMatrix(entity_handle_t handle,
                                     float fov, float aspect,
                                     float near, float far, float right,
                                     bool isOrtho) {
    LOGV("proj: %f %f %f %f %f %d", fov, aspect, near, far, right, isOrtho);
    cameraInfos[handle].fov = fov;
    cameraInfos[handle].aspect = aspect;
    cameraInfos[handle].near = near;
    cameraInfos[handle].far = far;
    cameraInfos[handle].right = right;
    cameraInfos[handle].isOrtho = isOrtho;
}

void WorldState::setWorldFrame(entity_handle_t handle,
                               float p0, float p1, float p2,
                               float fwd0, float fwd1, float fwd2,
                               float up0, float up1, float up2) {
    LOGV("worldframe: %f %f %f / %f %f %f / %f %f %f",
         p0, p1, p2,
         up0, up1, up2,
         fwd0, fwd1, fwd2);
    Entity &ent = entities[handle];
    ent.pos = makevector4(p0, p1, p2, 1.0f);
    ent.fwd = makevector4(fwd0, fwd1, fwd2, 0.0f);
    ent.up = makevector4(up0, up1, up2, 0.0f);
}

void WorldState::setScale(entity_handle_t handle,
                          float xscale, float yscale, float zscale) {
    LOGV("scale: %f %f %f",
         xscale, yscale, zscale);
    Entity &ent = entities[handle];
    ent.scale = makevector4(xscale, yscale, zscale, 1.0f);
}

void WorldState::setIntProp(entity_handle_t handle, const std::string &key, int val) {
    LOGV("%s %d", key.c_str(), val);

}

void WorldState::setFloatProp(entity_handle_t handle, const std::string &key, float val) {
    LOGV("%s %f", key.c_str(), val);
}

void
WorldState::setStringProp(entity_handle_t handle, const std::string &key, const std::string &val) {
    LOGV("%s %s", key.c_str(), val.c_str());
}

void WorldState::defineCurve(const std::string &name) {
    LOGV("%s: %s", __FUNCTION__, name.c_str());
    curves[name] = new BezierCurve();
}

void WorldState::setCurve(const std::string &name, int pointIndex,
                          const std::string &handleLeftType,
                          const std::string &handleRightType,
                          float hlx, float hly, float hlz,
                          float hrx, float hry, float hrz,
                          float x, float y, float z) {
    LOGV("%s %d %s %s (%f %f %f) (%f %f %f) (%f %f %f)",
         name.c_str(), pointIndex,
         handleLeftType.c_str(),
         handleRightType.c_str(),
         hlx, hly, hlz,
         hrx, hry, hrz,
         x, y, z);
    BezierCurve *c = curves[name];
    c->addPoint(pointIndex, hlx, hly, hlz, hrx, hry, hrz, x, y, z);
}

void WorldState::setCurveAction(const std::string &name,
                                const std::string &actName,
                                const std::string &actType,
                                const std::string &handleLeftType,
                                const std::string &handleRightType,
                                float hlx, float hly,
                                float hrx, float hry,
                                float x, float y) {
    LOGV("%s %s %s %s %s (%f %f) (%f %f) (%f %f)",
         name.c_str(),
         actName.c_str(),
         actType.c_str(),
         handleLeftType.c_str(),
         handleRightType.c_str(),
         hlx, hly,
         hrx, hry,
         x, y);
    ActionCurve &act = curves[name]->action();
    act.addKey(
            ActionCurve::curveTypeFromString(actType),
            ActionCurve::handleTypeFromString(handleLeftType),
            ActionCurve::handleTypeFromString(handleRightType),
            hlx, hly, hrx, hry, x, y);
}

void WorldState::defineParticles(const std::string &name) {
    LOGV("%s: %s", __FUNCTION__, name.c_str());
    particleSystems[name] = new ParticleSystem();

}

void WorldState::setParticles(const std::string &name,
                              int count, int begin,
                              int end, int lifetime,
                              float scale, float randomscale,
                              const std::string &followPath) {
    LOGV("%s: %s %d %d %d %d %f %f %s", __FUNCTION__, name.c_str(),
         begin, end, count, lifetime, scale, randomscale, followPath.c_str());
    ParticleSystem *p = particleSystems[name];
    p->setCountAndStartEnd(count, begin, end);
    p->lifetime = lifetime;
    p->scale = scale;
    p->randomScale = randomscale;
    p->followPath = curves[followPath];
}

void WorldState::setParticlesModel(const std::string &name,
                                   const std::string &modelName) {
    LOGV("%s: %s %s", __FUNCTION__,
         name.c_str(),
         modelName.c_str());

    if (namedRenderModels.find(modelName) ==
        namedRenderModels.end()) {
        defineModel(modelName);
    }

    ParticleSystem &p = *particleSystems[name];

    p.models.push_back(namedRenderModels[modelName]);
}


void WorldState::addRenderModel(const std::string &name) {
    renderModels.push_back(RenderModel());
    renderModels[renderModels.size() - 1].loadByBasename(name);

    namedRenderModels[name] = (render_state_handle_t) (renderModels.size() - 1);
}

void WorldState::addEntity(entity_handle_t handle, const std::string &name) {
    if (handle >= entities.size())
        entities.resize(handle + 1);

    // Note this sets the render model from the name.
    Entity &ent = entities[handle];
    ent.renderModel = namedRenderModels[name];
}

void WorldState::addCameraInfo(entity_handle_t handle, bool isLight) {
    if (handle >= cameraInfos.size())
        cameraInfos.resize(handle + 1);

    cameraInfos[handle].isLight = isLight;
    if (isLight) {
        lights.push_back(handle);
    }
}

void
WorldState::setAnimFrame(uint32_t framenum, uint32_t handle, float f1, float f2, float f3, float f4,
                         float f5, float f6, float f7, float f8, float f9, float f10, float f11,
                         float f12) {
    if (framenum >= animFrames.size())
        animFrames.resize(framenum + 1);

    animFrames[framenum].push_back({
                                           handle,
                                           makevector4(f1, f2, f3, 1.0f),
                                           makevector4(f4, f5, f6, 0.0f),
                                           makevector4(f7, f8, f9, 0.0f),
                                           makevector4(f10, f11, f12, 0.0f),
                                   });
    totalFrames = animFrames.size();
}

void WorldState::applyAnimFramesAt(uint32_t frame) {
    if (frame >= animFrames.size()) {
        currFrame = 0;
        return;
    }

    for (const auto &entUpdate : animFrames[frame]) {
        Entity &ent = entities[entUpdate.eid];
        ent.pos = entUpdate.pos;
        ent.fwd = entUpdate.fwd;
        ent.up = entUpdate.up;
        ent.scale = entUpdate.scale;
    }
}

// The huge update function!
bool WorldState::update() {
    //ScopedProfiler updateProfile("updateProfile");
    uint64_t now = currTimeUs();

    if (lastUpdateTime == 0) {
        startTime = now;
    }

    currFrame = ((now - startTime) / (uint64_t) 16667) % (uint64_t) animFrames.size();

    if (currFrame && (currFrame == lastFrame)) {
        return false;
    }

    if (currFrame < lastFrame) {
        done = true;
        uint32_t droppedFrames = totalFrames - framesShown;
        float dropRatio = (float) droppedFrames / (float) totalFrames;
        fps = 60.0f * (1.0f - dropRatio);
        LOGD("curr %d last %d anims %zu Result: Total frames: %u Dropped: %u Drop ratio: %f Avg fps: %f",
             currFrame, lastFrame, animFrames.size(),
             totalFrames, droppedFrames, dropRatio, fps);
        return false;
    }

    framesShown++;
    applyAnimFramesAt(currFrame);

    dyingIndices.clear();
    newIndices.resize(entities.size(), 0);

    // Update existing entities and their lifetimes.
    entity_handle_t i = 0;
    entity_handle_t death_offset = 0;
    for (auto &e : entities) {
        e.update(currFrame);
        if (!e.live) {
            dyingIndices.push_back(i);
            newIndices[i] = -1;
            death_offset++;
        } else {
            newIndices[i] = i - death_offset;
        }
        i++;
    }

    for (auto it : particleSystems) {
        ParticleSystem *p = it.second;
        p->setFrame(currFrame);
        p->updateParticlesToEntities(this);
        p->updateForDeadEntities(dyingIndices, newIndices);
    }

    // Collect all dead entities.
    entities.erase(
            std::remove_if(
                    entities.begin(), entities.end(),
                    [](const Entity &e) { return !e.live; }),
            entities.end());

    lastFrame = currFrame;
    lastUpdateTime = now;
    return true;
}

void WorldState::loadSkybox() {
    if (skyboxName.empty()) return;

    /* Texture target order for cube map
       GL_TEXTURE_CUBE_MAP_POSITIVE_X	Right
       GL_TEXTURE_CUBE_MAP_NEGATIVE_X	Left
       GL_TEXTURE_CUBE_MAP_POSITIVE_Y	Top
       GL_TEXTURE_CUBE_MAP_NEGATIVE_Y	Bottom
       GL_TEXTURE_CUBE_MAP_POSITIVE_Z	Back
       GL_TEXTURE_CUBE_MAP_NEGATIVE_Z	Front */

    // IN that order:
    std::vector<std::string> names = {
            "xpos.png", "xneg.png",
            "ypos.png", "yneg.png",
            "zpos.png", "zneg.png",
    };

    for (const auto &facename : names) {
        skyboxData.push_back(
                TextureLoader::get()->loadPNGAsRGBA8(
                        skyboxName + FILE_PATH_SEP + facename,
                        skyboxTexWidth, skyboxTexHeight));
        LOGV("%s: w h %u %u", __func__, skyboxTexWidth, skyboxTexHeight);
    }
}

