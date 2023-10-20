#pragma once
#include "ModelBufferManager.h"
#include "../include/RenderConst.h"
#include <chrono>

// TODO: Implement multiple models
ModelBufferManager::ModelBufferManager(Device* deviceInfo, CommandPool* commandPool,  std::vector<Model*> models)
{
    model = models[0];

    std::vector<Vertex> vertices = models[0]->getVertices();;
    std::vector<uint32_t> indices = models[0]->getIndices();

    createVertexBuffer(deviceInfo, commandPool, vertices);
    createIndexBuffer(deviceInfo, commandPool, indices);
    createUniformBuffer(deviceInfo);
}

void ModelBufferManager::cleanup(Device* deviceInfo)
{
    vertexBuffer->freeBuffer(deviceInfo);
    delete vertexBuffer;
    indexBuffer->freeBuffer(deviceInfo);
    delete indexBuffer;

    model->cleanup(deviceInfo);
    delete model;

    for (size_t i = 0; i < RenderConst::MAX_FRAMES_IN_FLIGHT; i++) {
        uniformBuffers[i]->freeBuffer(deviceInfo);
        delete uniformBuffers[i];
    }
}

//TODO: createVertexBuffer & createIndexBuffer shares a lot of code
void ModelBufferManager::createVertexBuffer(Device* deviceInfo, CommandPool* commandPool, std::vector<Vertex> vertices)
{
    VkDeviceSize vertexBufferSize = sizeof(Vertex) * vertices.size();

    Buffer* vertexStagingBuffer = new Buffer(deviceInfo, vertexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    vertexStagingBuffer->copyData(deviceInfo, vertices.data(), 0, vertexBufferSize, 0);

    vertexBuffer = new Buffer(deviceInfo, vertexBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    vertexBuffer->copyBuffer(deviceInfo, commandPool, vertexStagingBuffer, vertexBufferSize);

    vertexStagingBuffer->freeBuffer(deviceInfo);
}

void ModelBufferManager::createIndexBuffer(Device* deviceInfo, CommandPool* commandPool, std::vector<uint32_t> indices)
{
    VkDeviceSize indicesBufferSize = sizeof(uint32_t) * indices.size();

    Buffer* indexStagingBuffer = new Buffer(deviceInfo, indicesBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    indexStagingBuffer->copyData(deviceInfo, indices.data(), 0, indicesBufferSize, 0);

    indexBuffer = new Buffer(deviceInfo, indicesBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    indexBuffer->copyBuffer(deviceInfo, commandPool, indexStagingBuffer, indicesBufferSize);

    indexStagingBuffer->freeBuffer(deviceInfo);
}

void ModelBufferManager::createUniformBuffer(Device* deviceInfo)
{
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);
    
    uniformBuffers.resize(RenderConst::MAX_FRAMES_IN_FLIGHT);
    uniformBuffersMapped.resize(RenderConst::MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < RenderConst::MAX_FRAMES_IN_FLIGHT; i++) {
        uniformBuffers[i] = new Buffer(deviceInfo, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        
        vkMapMemory(deviceInfo->getLogicalDevice(), uniformBuffers[i]->getBufferMemory(), 0, bufferSize, 0, &uniformBuffersMapped[i]);
    }
}

void ModelBufferManager::UpdateUniformBuffer(Device* deviceInfo, VkExtent2D swapchainExtent, uint32_t index) 
{
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    UniformBufferObject ubo{};
    ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.proj = glm::perspective(glm::radians(45.0f), swapchainExtent.width / (float)swapchainExtent.height, 0.1f, 10.0f);
    ubo.proj[1][1] *= -1;

    memcpy(uniformBuffersMapped[index], &ubo, sizeof(ubo));
}

std::vector<Buffer*> ModelBufferManager::getUniformBuffers()
{
    return uniformBuffers;
}

Buffer* ModelBufferManager::getUniformBuffer(uint32_t index)
{
    return uniformBuffers[index];
}

Model* ModelBufferManager::getModels(uint32_t index)
{
    return model;
}

VkBuffer& ModelBufferManager::getVertexBuffer()
{
    return vertexBuffer->getBuffer();
}

VkBuffer& ModelBufferManager::getIndexBuffer()
{
    return indexBuffer->getBuffer();
}

uint32_t ModelBufferManager::getIndexSize()
{
    return static_cast<uint32_t>(model->getIndices().size());
}
