#pragma once
#include "ModelManager.h"
#include "../include/RenderConst.h"
#include "../Descriptors/Types/UBO/UBO.h"
#include "../Descriptors/DescriptorUtils.h"
#include "../Math/MathUtils.h"
#include "../Image/ImageUtils.h"

ModelManager::ModelManager(Device* deviceInfo)
{
    cameraMappedBufferManager = new MappedBufferManager(deviceInfo, RenderConst::MAX_FRAMES_IN_FLIGHT, sizeof(GPUCameraData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    sceneMappedBufferManager = new MappedBufferManager(deviceInfo, RenderConst::MAX_FRAMES_IN_FLIGHT, (RenderConst::MAX_FRAMES_IN_FLIGHT * DescriptorUtils::padUniformBufferSize(sizeof(GPUSceneData), deviceInfo->getGPUProperties().limits.minUniformBufferOffsetAlignment) * RenderConst::MAX_FRAMES_IN_FLIGHT), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
}

void ModelManager::cleanup(Device* deviceInfo)
{
    for (auto vertexBuffer : vertexBuffers) {
        vertexBuffer->freeBuffer(deviceInfo);
        delete vertexBuffer;
    }

    indexBuffer->freeBuffer(deviceInfo);
    delete indexBuffer;

    for (auto model : models) {
        model->cleanup();
        delete model;
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

    indexBuffer = new Buffer(deviceInfo, indicesBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    indexBuffer->copyBuffer(deviceInfo, commandPool, indexStagingBuffer, indicesBufferSize);

    indexStagingBuffer->freeBuffer(deviceInfo);
    delete indexStagingBuffer;
}

void ModelManager::bindBuffers(const VkCommandBuffer& commandBuffer, const int32_t index)
{
    VkDeviceSize offsets[] = { 0 };
    
    auto buff = vertexBuffers[index]->getBuffer();
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &buff, offsets);
    vkCmdBindIndexBuffer(commandBuffer, indexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32);
}

void ModelManager::updatePushConstants(VkCommandBuffer& commandBuffer, VkPipelineLayout& layout, const int32_t index)
{
    vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstantData), &modelData[index]);
}

void ModelManager::drawIndexed(const VkCommandBuffer& commandBuffer, const uint32_t& index)
{
    vkCmdDrawIndexed(commandBuffer, models[index]->getMesh()->getIndices().size(), 1, 0, 0, 0);
}

void ModelManager::updateUniformBuffer(Device* deviceInfo, Camera* camera, const SceneInfo* sceneInfo, const uint32_t& mappedBufferManagerIndex, const uint32_t& bufferIndex)
{
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
    scn.ambientColor = sceneInfo->ambientColor;
    //TODO: Support Multiple Lights
    scn.sunlightDirection = sceneInfo->directionalLights[0].lightDirection;
    scn.sunlightColor = sceneInfo->directionalLights[0].lightColor;
    scn.cameraPosition = sceneInfo->cameras[0].position;

    //

    memcpy(cameraMappedBufferManager->getMappedBuffer(bufferIndex), &ubo, sizeof(ubo));
    //TODO: Investigate X buffers at index or 1 buffer with offset
    // char pointer allows for easily applying an offset
    //char* cp = reinterpret_cast<char*>(sceneMappedBufferManager->getMappedBuffer(bufferIndex));
    //memcpy(cp + bufferIndex * DescriptorUtils::padUniformBufferSize(sizeof(GPUSceneData), deviceInfo->getGPUProperties().limits.minUniformBufferOffsetAlignment), &scn, sizeof(scn));
    memcpy(sceneMappedBufferManager->getMappedBuffer(bufferIndex), &scn, sizeof(scn));
}

MappedBufferManager* ModelManager::getMappedBufferManager(uint32_t index)
{
    switch (index) {
    case 0: return cameraMappedBufferManager;
    case 1: return sceneMappedBufferManager;
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
        models.push_back(new Model(modelCreateInfos[i], getTextureIndex(modelCreateInfos[i].texturePath)));
        modelData.push_back(PushConstantData(glm::vec4(models[i]->getTextureIndex(), i, 0, 0)));
    }

    for (auto model : models) {
        auto verts = model->getMesh()->getVertices();
        auto inds = model->getMesh()->getIndices();
        createVertexBuffer(deviceInfo, commandPool, verts);
        createIndexBuffer(deviceInfo, commandPool, inds);
    }
}

void ModelManager::loadTextures(Device* deviceInfo, CommandPool* commandPool, const std::vector<MaterialInfo>& materials)
{
    matInfos = materials;
    textures = ImageUtils::createTexturesFromCreateInfo(deviceInfo, commandPool, materials);
}

std::vector<Image*>& ModelManager::getTextures()
{
    return textures;
}

//TODO: Implement more optimized search like std::unordered_map with custom hash
uint32_t ModelManager::getTextureIndex(const char* path)
{
    for (uint32_t i = 0; i < matInfos.size(); i++) {
        if (matInfos[i].fileName == path)
            return i;
    }
    return 0;
}