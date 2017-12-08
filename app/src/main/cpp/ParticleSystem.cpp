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

#include "ParticleSystem.h"

#include "log.h"

#include <algorithm>
#include <cstdlib>

struct RandomState {
    bool initialized = false;
    int randomSeed;
};

static RandomState sRandomState;

static void sInitRandom() {
    if (sRandomState.initialized) return;

    sRandomState.randomSeed = 0xBA5EFEA2;
    srand(sRandomState.randomSeed);
    sRandomState.initialized = true;
}

// Yes, these are not great but they make stuff that looks
// different
static int sUniformInteger(int start, int end) {
    if (start == end) return start;

    int r = rand() % (end - start);
    return start + r;
}

static float sUniformFloat(float start, float end) {
    float num = (float) rand();
    float den = (float) RAND_MAX;
    float frac = num / den;

    return start + (end - start) * frac;
}

static bool sFlip(float prob) {
    if (sUniformFloat(0, 1) < prob) return true;
    return false;
}

static vector4 sRandomDirection() {
    float x = (sFlip(0.5) ? -1.0f : 1.0f) * sUniformFloat(0.1f, 1.0f);
    float y = (sFlip(0.5) ? -1.0f : 1.0f) * sUniformFloat(0.1f, 1.0f);
    float z = (sFlip(0.5) ? -1.0f : 1.0f) * sUniformFloat(0.1f, 1.0f);
    return v4normed(makevector4(x, y, z, 0));
}

template<class T>
static T &sRandomSelect(std::vector<T> &items) {
    return items[sUniformInteger(0, items.size() - 1)];
}

ParticleSystem::ParticleSystem() {
    sInitRandom();
}

void ParticleSystem::setCountAndStartEnd(int count_in, int begin_in, int end_in) {
    begin = begin_in;
    end = end_in;
    count = count_in;
    mCurrentCount = count_in;

    int onInterval = end - begin;

    mSpawnInterval = onInterval / count;
    if (!mSpawnInterval) {

        mMultiParticleSpawn = true;
        mSpawnInterval = 1;
        mParticlesPerFrame = count / onInterval;

        mParticlesPerFrameBase = int(mParticlesPerFrame);

        float remainder =
                mParticlesPerFrame - mParticlesPerFrameBase;

        int extraParticleEvents =
                remainder * onInterval;

        if (!extraParticleEvents) return;

        mAdditionalParticleInterval =
                onInterval / extraParticleEvents;
    }
}

void ParticleSystem::setFrame(int frame) {
    mFrame = frame;
}

void ParticleSystem::updateParticlesToEntities(WorldState *world) {
    if (mFrame >= begin && mFrame <= end) {
        for (int i = mLastFrame + 1; i <= mFrame; i++) {
            if (mCurrentCount) {
                if (mMultiParticleSpawn) {
                    for (int j = 0; j < mParticlesPerFrameBase; j++) {
                        mCurrentCount--;
                        spawnParticle(world, i - mFrame);
                    }

                    if (mAdditionalParticleInterval &&
                        ((i - begin) % mAdditionalParticleInterval == 0)) {
                        mCurrentCount--;
                        spawnParticle(world, i - mFrame);
                    }
                } else {
                    if (((i - begin) % mSpawnInterval == 0)) {
                        mCurrentCount--;
                        spawnParticle(world, i - mFrame);
                    }
                }
            }
        }
    }
    updateParticles(world);

    mLastFrame = mFrame;
}

void ParticleSystem::spawnParticle(WorldState *world, int lifeOffset) {
    entity_handle_t newHandle = world->spawnEntity();
    mLiveParticles.push_back(newHandle);
    initParticle(world->entities[newHandle], lifeOffset);
}

void ParticleSystem::updateParticles(WorldState *world) {
    for (const auto handle : mLiveParticles) {
        updateParticle(world->entity(handle));
    }
}

void ParticleSystem::updateForDeadEntities(const std::vector<entity_handle_t> &dyingIndices,
                                           const std::vector<entity_handle_t> &newIndices) {
    mLiveParticles.erase(
            std::remove_if(
                    mLiveParticles.begin(), mLiveParticles.end(),
                    [&newIndices](const entity_handle_t i) {
                        if (i < newIndices.size()) {
                            return newIndices[i] == -1;
                        } else {
                            // must be just added, then we only need
                            // adjust by death offset and leave it
                            return false;
                        }
                    }),
            mLiveParticles.end());

    int death_offset = dyingIndices.size();
    for (auto &handle : mLiveParticles) {
        if (handle < newIndices.size()) {
            handle = newIndices[handle];
        } else {
            handle -= death_offset;
        }
    }
}

void ParticleSystem::initParticle(Entity &entity, int lifeOffset) {
    entity.framesToLive = lifetime + lifeOffset;
    entity.renderModel = sRandomSelect(models);

    float randomOffX = sUniformFloat(-0.3f, 0.3f);
    float randomOffY = sUniformFloat(-0.3f, 0.3f);
    float randomOffZ = sUniformFloat(-0.3f, 0.3f);

    entity.initialOffset =
            makevector4(
                    randomOffX, randomOffY, randomOffZ, 1);

    entity.spinAxis = sRandomDirection();
    entity.spinPeriod = 60 * sUniformFloat(0.8, 1.2);

    entity.fwd = sRandomDirection();
    entity.up = sRandomDirection();

    float randomScalePart = scale + scale * randomScale * sUniformFloat(-0.5f, 0.5f);
    entity.scale =
            makevector4(
                    randomScalePart,
                    randomScalePart,
                    randomScalePart,
                    0);
}

void ParticleSystem::updateParticle(Entity &entity) {
    if (!followPath) return;

    // int particleFrame = begin + (lifetime - entity.framesToLive);
    // float pathProgress =
    //     followPath->action().evalAtFrame(particleFrame, true /* normalized */);

    int entityFrames =
            2 * (lifetime - entity.framesToLive);
    float pathProgress =
            entityFrames / (float) lifetime;

    if (pathProgress >= 0.995) return;

    followPath->evalArclen(pathProgress,
                           &entity.pos.x,
                           &entity.pos.y,
                           &entity.pos.z);
    entity.pos = entity.pos + entity.initialOffset;

    float angle = 2 * 3.14159265358979;
    angle /= (float) entity.spinPeriod;
    entity.applyRotation(entity.spinAxis, angle);
}
