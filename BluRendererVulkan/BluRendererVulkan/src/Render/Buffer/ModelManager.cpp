#pragma once
#include "ModelManager.h"
#include "../include/RenderConst.h"
#include "../Descriptors/DescriptorUtils.h"
#include "../Math/MathUtils.h"
#include "../Image/ImageUtils.h"

ModelManager::ModelManager(Device* deviceInfo)
{
    cameraMappedBufferManager = new MappedBufferManager(deviceInfo, RenderConst::MAX_FRAMES_IN_FLIGHT, sizeof(GPUCameraData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    sceneMappedBufferManager = new MappedBufferManager(deviceInfo, RenderConst::MAX_FRAMES_IN_FLIGHT, sizeof(GPUSceneData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    materialMappedBufferManager = new MappedBufferManager(deviceInfo, RenderConst::MAX_FRAMES_IN_FLIGHT, sizeof(GPUMaterialData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
}

void ModelManager::cleanup(Device* deviceInfo)
{
    for (auto vertexBuffer : vertexBuffers) {
        vertexBuffer->freeBuffer(deviceInfo);
        delete vertexBuffer;
    }

    for (auto indexBuffer : indexBuffers) {
        indexBuffer->freeBuffer(deviceInfo);
        delete indexBuffer;
    }

    for (auto model : models) {
        model->cleanup();
        delete model;
    }

    for (auto texture : textures) {
        texture.cleanup(deviceInfo);
    }

    cameraMappedBufferManager->cleanup(deviceInfo);
    delete cameraMappedBufferManager;
}

//TODO: createVertexBuffer & createIndexBuffer shares a lot of code
void ModelManager::createVertexBuffer(Device* deviceInfo, CommandPool* commandPool, std::vector<Vertex> vertices)
{
    VkDeviceSize vertexBufferSize = sizeof(Vertex) * vertices.size();

    Buffer* vertexStagingBuffer = new Buffer(deviceInfo, vertexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    vertexStagingBuffer->copyData(deviceInfo, vertices.data(), 0, vertexBufferSize, 0);

    Buffer* vertBuffer = new Buffer(deviceInfo, vertexBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    vertBuffer->copyBuffer(deviceInfo, commandPool, vertexStagingBuffer, vertexBufferSize);
    
    vertexBuffers.push_back(vertBuffer);

    vertexStagingBuffer->freeBuffer(deviceInfo);
    delete vertexStagingBuffer;
}

void ModelManager::createIndexBuffer(Device* deviceInfo, CommandPool* commandPool, std::vector<uint32_t> indices)
{
    VkDeviceSize indicesBufferSize = sizeof(uint32_t) * indices.size();

    Buffer* indexStagingBuffer = new Buffer(deviceInfo, indicesBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    indexStagingBuffer->copyData(deviceInfo, indices.data(), 0, indicesBufferSize, 0);

    Buffer* indexBuffer = new Buffer(deviceInfo, indicesBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    indexBuffer->copyBuffer(deviceInfo, commandPool, indexStagingBuffer, indicesBufferSize);

    indexBuffers.push_back(indexBuffer);

    indexStagingBuffer->freeBuffer(deviceInfo);
    delete indexStagingBuffer;
}

void ModelManager::bindBuffers(const VkCommandBuffer& commandBuffer, const int32_t index)
{
    VkDeviceSize offsets[] = { 0 };
    
    auto vertBuff = vertexBuffers[index]->getBuffer();
    auto indBuff = indexBuffers[index]->getBuffer();
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertBuff, offsets);
    vkCmdBindIndexBuffer(commandBuffer, indBuff, 0, VK_INDEX_TYPE_UINT32);
}

void ModelManager::updatePushConstants(VkCommandBuffer& commandBuffer, VkPipelineLayout& layout, const int32_t index)
{
    vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstantData), &modelData[index]);
}

void ModelManager::drawIndexed(const VkCommandBuffer& commandBuffer, const uint32_t& index)
{
    vkCmdDrawIndexed(commandBuffer, models[index]->getMesh()->getIndices().size(), 1, 0, 0, 0);
}

void ModelManager::updateUniformBuffer(Device* deviceInfo, Camera* camera, const SceneInfo* sceneInfo, const uint32_t& mappedBufferManagerIndex, const uint32_t& bufferIndex) {
    // Vertex
    GPUCameraData ubo{};
    ubo.view = camera->getViewMat();
    ubo.proj = camera->getProjMat();

    auto modelCount = getModelCount();

    for (int i = 0; i < modelCount; i++)
    {
        ubo.model[i] = MathUtils::ApplyTransformAndRotation(models[i]->getPosition(), glm::vec3(0));
    }

    // Frag
    GPUSceneData scn{};
    int numOfLights = sceneInfo->lights.size();
    for (uint32_t i = 0; i < numOfLights; i++)
    {
        scn.lightInfo[i] = LightInfo(sceneInfo->lights[i].lightType, sceneInfo->lights[i].lightPosition, sceneInfo->lights[i].lightRotation, sceneInfo->lights[i].lightColor, sceneInfo->lights[i].constant, sceneInfo->lights[i].linear, sceneInfo->lights[i].quad, sceneInfo->lights[i].innerCutoff, sceneInfo->lights[i].outerCutoff);
    }
    scn.ambientColor = sceneInfo->ambientColor;
    scn.cameraPosition = glm::vec4(sceneInfo->cameras[0].position, numOfLights);

    memcpy(cameraMappedBufferManager->getMappedBuffer(bufferIndex), &ubo, sizeof(ubo));
    
    memcpy(sceneMappedBufferManager->getMappedBuffer(bufferIndex), &scn, sizeof(scn));
}

//Index 0 Camera
//Index 1 Scene
//Index 2 Material
MappedBufferManager* ModelManager::getMappedBufferManager(uint32_t index) {
    switch (index) {
    case 0: return cameraMappedBufferManager;
    case 1: return sceneMappedBufferManager;
    case 2: return materialMappedBufferManager;
    default: return nullptr;
    }
}

int ModelManager::getModelCount()
{
    return models.size();
}

//TODO: Support Destroying Models
void ModelManager::loadModels(Device* deviceInfo, CommandPool* commandPool, const std::vector<SceneModel> modelCreateInfos) {
    for (uint32_t i = 0; i < modelCreateInfos.size(); i++) {
        models.push_back(new Model(modelCreateInfos[i], getTextureIndex(modelCreateInfos[i].texturePath), modelCreateInfos[i].materialIndex));
        modelData.push_back(PushConstantData(glm::vec4(models[i]->getTextureIndex(), i, models[i]->getMaterialIndex(), 0)));
    }

    for (auto model : models) {
        auto verts = model->getMesh()->getVertices();
        auto inds = model->getMesh()->getIndices();
        createVertexBuffer(deviceInfo, commandPool, verts);
        createIndexBuffer(deviceInfo, commandPool, inds);
    }
}

void ModelManager::loadTextures(Device* deviceInfo, CommandPool* commandPool, const std::vector<TextureInfo>& textures) {
    textureInfos = textures;

    for(auto& tex : textures)
    {
        this->textures.push_back(TextureData(ImageUtils::createImageFromPath(deviceInfo, commandPool, (tex.fileName + tex.fileType).c_str()), ImageUtils::createImageFromPath(deviceInfo, commandPool, (tex.fileName + "_specular" + tex.fileType).c_str()), ImageUtils::createImageFromPath(deviceInfo, commandPool, (tex.fileName + "_diffuse" + tex.fileType).c_str())));
    }
}

std::vector<TextureData>& ModelManager::getTextures() {
    return textures;
}

//TODO: Implement more optimized search like std::unordered_map with custom hash
uint32_t ModelManager::getTextureIndex(const char* path) {
    for (uint32_t i = 0; i < textureInfos.size(); i++) {
        if (textureInfos[i].fileName == path)
            return i * 3;
    }
    return 0;
}