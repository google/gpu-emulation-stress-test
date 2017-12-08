//
// Created by Lingfeng Yang on 3/24/17.
//

#pragma once

#include "BezierCurve.h"
#include "Entity.h"
#include "ParticleSystem.h"
#include "RenderModel.h"

#include <string>
#include <unordered_map>
#include <vector>

typedef uint32_t entity_handle_t;

class BezierCurve;

class ParticleSystem;

class WorldState {
public:
    struct CameraInfo {
        float fov;
        float aspect;
        float near;
        float far;
        float right; // Ortho scale
        bool isLight;
        bool isOrtho;
    };

    WorldState() = default;

    void loadFromFile(const std::string &filename, int numObjects);

    void resetAspectRatio(int width, int height);

    void defineCameraOrLight(const std::string &name, entity_handle_t handle, bool isLight = false);

    void defineModel(const std::string &name);

    void defineEntity(const std::string &name, entity_handle_t handle);

    void setProjectionMatrix(entity_handle_t handle,
                             float fov, float aspect,
                             float near, float far,
                             float right, bool isOrtho);

    void setWorldFrame(entity_handle_t handle,
                       float p0, float p1, float p2,
                       float fwd0, float fwd1, float fwd2,
                       float up0, float up1, float up2);

    void setScale(entity_handle_t handle,
                  float xscale, float yscale, float zscale);

    void setIntProp(entity_handle_t handle, const std::string &key, int val);

    void setFloatProp(entity_handle_t handle, const std::string &key, float val);

    void setStringProp(entity_handle_t handle, const std::string &key, const std::string &val);

    void defineCurve(const std::string &name);

    void setCurve(const std::string &name, int pointIndex,
                  const std::string &handleLeftType,
                  const std::string &handleRightType,
                  float hlx, float hly, float hlz,
                  float hrx, float hry, float hrz,
                  float x, float y, float z);

    void setCurveAction(const std::string &name,
                        const std::string &actName,
                        const std::string &actType,
                        const std::string &handleLeftType,
                        const std::string &handleRightType,
                        float hlx, float hly,
                        float hrx, float hry,
                        float x, float y);

    void defineParticles(const std::string &name);

    void setParticles(const std::string &name,
                      int count, int begin,
                      int end, int lifetime,
                      float scale, float randomscale,
                      const std::string &followPath);

    void setParticlesModel(const std::string &name,
                           const std::string &modelName);

    bool update();

    std::vector<RenderModel> renderModels;
    std::unordered_map<std::string, render_state_handle_t> namedRenderModels;

    entity_handle_t currentCamera = 0;

    entity_handle_t currentLight = 0;
    std::vector<entity_handle_t> lights = {};

    // Entity management
    std::vector<Entity> entities;
    // When entities are collected, users of WorldState
    // may need to update their indices. This tracks how.
    std::vector<entity_handle_t> dyingIndices;
    std::vector<entity_handle_t> newIndices;

    Entity &entity(entity_handle_t index) { return entities[index]; }

    entity_handle_t spawnEntity() {
        entity_handle_t res = entities.size();
        entities.emplace_back();
        return res;
    }

    std::vector<CameraInfo> cameraInfos;
    std::unordered_map<std::string, entity_handle_t> namedEntityMap;

    // For effects
    std::unordered_map<std::string, BezierCurve *> curves;
    std::unordered_map<std::string, BezierCurve *> curveActions;
    std::unordered_map<std::string, ParticleSystem *> particleSystems;

    uint64_t lastUpdateTime = 0;
    uint32_t currFrame = 0;

    // Skybox
    std::string skyboxName = "";
    std::vector<std::vector<unsigned char> > skyboxData;
    uint32_t skyboxTexWidth;
    uint32_t skyboxTexHeight;

    void loadSkybox();

    // Benchmark stuff
    uint32_t totalFrames = 0;
    uint32_t lastFrame = 0;
    uint32_t framesShown = 0;
    float fps = 0.0f;
    bool done = false;

private:
    void addRenderModel(const std::string &name);

    void addEntity(entity_handle_t handle, const std::string &name = "");

    void addCameraInfo(entity_handle_t handle, bool isLight);

    void
    setAnimFrame(uint32_t framenum, uint32_t handle, float f1, float f2, float f3, float f4,
                 float f5,
                 float f6, float f7, float f8, float f9, float f10, float f11, float f12);

    uint64_t startTime;

    void applyAnimFramesAt(uint32_t frame);

    struct EntityPose {
        entity_handle_t eid;
        vector4 pos;
        vector4 fwd;
        vector4 up;
        vector4 scale;
    };

    std::vector<std::vector<EntityPose> > animFrames;
};