#pragma once
#include "ModelManager.h"
#include "../include/RenderConst.h"
#include "../Descriptors/Types/UBO/UBO.h"
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
    createUniformBuffer(deviceInfo);
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

    for (size_t i = 0; i < RenderConst::MAX_FRAMES_IN_FLIGHT; i++) {
        uniformBuffers[i]->freeBuffer(deviceInfo);
        delete uniformBuffers[i];
    }
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

void ModelManager::createUniformBuffer(Device* deviceInfo)
{
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);
    
    uniformBuffers.resize(RenderConst::MAX_FRAMES_IN_FLIGHT);
    uniformBuffersMapped.resize(RenderConst::MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < RenderConst::MAX_FRAMES_IN_FLIGHT; i++) {
        uniformBuffers[i] = new Buffer(deviceInfo, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        
        vkMapMemory(deviceInfo->getLogicalDevice(), uniformBuffers[i]->getBufferMemory(), 0, bufferSize, 0, &uniformBuffersMapped[i]);
    }
}

void ModelManager::updatePushConstants(VkCommandBuffer& commandBuffer, VkPipelineLayout& layout)
{
    uint32_t modelCount = static_cast<uint32_t>(modelData.size());
    for (uint32_t i = 0; i < modelCount; i++) {
        vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstantData), &modelData[i]);
    }
}

void ModelManager::updateUniformBuffer(Device* deviceInfo, Camera* camera, uint32_t index)
{
    UniformBufferObject ubo{};
    //
    //ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));

    // Camera View
    ubo.view = camera->getViewMat();
    // Camera Proj
    ubo.proj = camera->getProjMat();

    memcpy(uniformBuffersMapped[index], &ubo, sizeof(ubo));
}

std::vector<Buffer*> ModelManager::getUniformBuffers()
{
    return uniformBuffers;
}

Buffer* ModelManager::getUniformBuffer(uint32_t index)
{
    return uniformBuffers[index];
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
