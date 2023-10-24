#pragma once
#include "ModelManager.h"
#include "../include/RenderConst.h"
#include "../Descriptors/Types/UBO/UBO.h"
#include "../Descriptors/DescriptorUtils.h"
#include "../Math/MathUtils.h"

ModelManager::ModelManager(Device* deviceInfo, CommandPool* commandPool, const std::vector<ModelCreateInfo> modelCreateInfos)
{
    for (const ModelCreateInfo& modelCreateInfo : modelCreateInfos) {
        models.push_back(new Model(deviceInfo, commandPool, modelCreateInfo));
        modelData.push_back(PushConstantData(MathUtils::ApplyTransformAndRotation(modelCreateInfo.pos, modelCreateInfo.rot, glm::mat4(1.0f))));
    }
    
    for (auto model : models) {
        auto verts = model->getMesh()->getVertices();
        auto inds = model->getMesh()->getIndices();
        vertices.insert(vertices.end(), verts.begin(), verts.end());
        indices.insert(indices.end(), inds.begin(), inds.end());
    }

    createVertexBuffer(deviceInfo, commandPool, vertices);
    createIndexBuffer(deviceInfo, commandPool, indices);

    cameraMappedBufferManager = new MappedBufferManager(deviceInfo, RenderConst::MAX_FRAMES_IN_FLIGHT, sizeof(GPUCameraData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    sceneMappedBufferManager = new MappedBufferManager(deviceInfo, RenderConst::MAX_FRAMES_IN_FLIGHT, (RenderConst::MAX_FRAMES_IN_FLIGHT * DescriptorUtils::padUniformBufferSize(sizeof(GPUSceneData), deviceInfo->getGPUProperties().limits.minUniformBufferOffsetAlignment) * RenderConst::MAX_FRAMES_IN_FLIGHT), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
}

void ModelManager::cleanup(Device* deviceInfo)
{
    vertexBuffer->freeBuffer(deviceInfo);
    delete vertexBuffer;
    indexBuffer->freeBuffer(deviceInfo);
    delete indexBuffer;

    for (auto model : models) {
        model->cleanup(deviceInfo);
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

    vertexBuffer = new Buffer(deviceInfo, vertexBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    vertexBuffer->copyBuffer(deviceInfo, commandPool, vertexStagingBuffer, vertexBufferSize);

    vertexStagingBuffer->freeBuffer(deviceInfo);
}

void ModelManager::createIndexBuffer(Device* deviceInfo, CommandPool* commandPool, std::vector<uint32_t> indices)
{
    VkDeviceSize indicesBufferSize = sizeof(uint32_t) * indices.size();

    Buffer* indexStagingBuffer = new Buffer(deviceInfo, indicesBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    indexStagingBuffer->copyData(deviceInfo, indices.data(), 0, indicesBufferSize, 0);

    indexBuffer = new Buffer(deviceInfo, indicesBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    indexBuffer->copyBuffer(deviceInfo, commandPool, indexStagingBuffer, indicesBufferSize);

    indexStagingBuffer->freeBuffer(deviceInfo);
}

void ModelManager::bindBuffers(const VkCommandBuffer& commandBuffer)
{
    VkBuffer vertexBuffers[] = { vertexBuffer->getBuffer() };
    VkDeviceSize offsets[] = { 0 };

    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

    vkCmdBindIndexBuffer(commandBuffer, indexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32);
}

void ModelManager::updatePushConstants(VkCommandBuffer& commandBuffer, VkPipelineLayout& layout)
{
    uint32_t modelCount = static_cast<uint32_t>(modelData.size());
    for (uint32_t i = 0; i < modelCount; i++) {
        vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstantData), &modelData[i]);
    }
}

void ModelManager::drawIndexed(const VkCommandBuffer& commandBuffer)
{
    vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
}

void ModelManager::updateUniformBuffer(Device* deviceInfo, Camera* camera, const uint32_t& mappedBufferManagerIndex, const uint32_t& bufferIndex)
{

    GPUCameraData ubo{};
    ubo.view = camera->getViewMat();
    ubo.proj = camera->getProjMat();

    GPUSceneData scn{};
    scn.ambientColor = glm::vec4(1, 1, 0.8, 1);

    memcpy(cameraMappedBufferManager->getMappedBuffer(bufferIndex), &ubo, sizeof(ubo));
    // char pointer allows for easily applying an offset
    char* cp = reinterpret_cast<char*>(sceneMappedBufferManager->getMappedBuffer(bufferIndex));
    memcpy(cp + bufferIndex * DescriptorUtils::padUniformBufferSize(sizeof(GPUSceneData), deviceInfo->getGPUProperties().limits.minUniformBufferOffsetAlignment), &scn, sizeof(scn));
}

MappedBufferManager* ModelManager::getMappedBufferManager(uint32_t index)
{
    switch (index) {
    case 0: return cameraMappedBufferManager;
    case 1: return sceneMappedBufferManager;
    default: return nullptr;
    }
}

Model* ModelManager::getModel(uint32_t index)
{
    return models[index];
}

VkBuffer& ModelManager::getVertexBuffer()
{
    return vertexBuffer->getBuffer();
}

VkBuffer& ModelManager::getIndexBuffer()
{
    return indexBuffer->getBuffer();
}

uint32_t ModelManager::getIndexSize()
{
    return static_cast<uint32_t>(indices.size());
}
