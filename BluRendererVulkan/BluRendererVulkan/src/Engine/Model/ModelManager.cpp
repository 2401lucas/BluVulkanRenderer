#pragma once
#include "ModelManager.h"
#include "../Math/MathUtils.h"

ModelManager::ModelManager(Device* deviceInfo)
{
    
}

void ModelManager::cleanup(Device* deviceInfo) {
    deleteAllModels();
}

uint32_t ModelManager::addModel(const SceneModel& modelCreateInfo) {
    models.push_back(new Model(modelCreateInfo, getTextureIndex(modelCreateInfo.texturePath), getTextureType(modelCreateInfo.texturePath)));
}

void ModelManager::deleteAllModels() {
    for (auto model : models) {
        model->cleanup();
        delete model;
    }
    models.clear();
}

std::list<Model*> ModelManager::getModels()
{
    return models;
}

void ModelManager::deleteModel(Model* model) {
    models.remove(model);
}

