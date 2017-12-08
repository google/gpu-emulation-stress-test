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

#include "Entity.h"
#include "WorldState.h"
#include "BezierCurve.h"

#include <vector>

typedef uint32_t entity_handle_t;

class WorldState;

class ParticleSystem {
public:
    ParticleSystem();

    int count;
    int begin;
    int end;
    int lifetime;
    float scale;
    float randomScale;

    BezierCurve *followPath = nullptr;

    std::vector<render_state_handle_t> models;

    void setCountAndStartEnd(int count_in, int begin, int end);

    void setFrame(int frame);

    void updateParticlesToEntities(WorldState *world);

    void updateForDeadEntities(const std::vector<entity_handle_t> &dyingIndices,
                               const std::vector<entity_handle_t> &newIndices);

private:
    void spawnParticle(WorldState *world, int lifeOffset);

    void updateParticles(WorldState *world);

    void initParticle(Entity &entity, int lifeOffset);

    void updateParticle(Entity &entity);

    int mCurrentCount = 0;
    int mSpawnInterval = 0;

    bool mMultiParticleSpawn = false;
    float mParticlesPerFrame = 1.0f;
    int mParticlesPerFrameBase = 1;
    int mAdditionalParticleInterval = 0;

    int mFrame = 0;
    int mLastFrame = 0;
    int mParticlesStartIndex = 0;

    std::vector<entity_handle_t> mLiveParticles;
};