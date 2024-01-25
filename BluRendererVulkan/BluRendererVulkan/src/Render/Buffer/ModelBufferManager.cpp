#include "ModelBufferManager.h"
#include "../Math/MathUtils.h"

ModelBufferManager::ModelBufferManager(Device* deviceInfo)
{
    cameraMappedBufferManager = new MappedBufferManager(deviceInfo, RenderConst::MAX_FRAMES_IN_FLIGHT, sizeof(GPUCameraData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    sceneMappedBufferManager = new MappedBufferManager(deviceInfo, RenderConst::MAX_FRAMES_IN_FLIGHT, sizeof(GPUSceneData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    materialMappedBufferManager = new MappedBufferManager(deviceInfo, RenderConst::MAX_FRAMES_IN_FLIGHT, sizeof(GPUMaterialData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    vertexBufferAllocator = new BufferAllocator(deviceInfo, 1048576/*1MB*/, 10485760/*1GB*/, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    indexBufferAllocator =  new BufferAllocator(deviceInfo, 1048576/*1MB*/, 10485760/*256MB*/, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
}

void ModelBufferManager::cleanup(Device* deviceInfo)
{
    vertexBufferAllocator->cleanup(deviceInfo);
    delete vertexBufferAllocator;

    indexBufferAllocator->cleanup(deviceInfo);
    delete indexBufferAllocator;

    cameraMappedBufferManager->cleanup(deviceInfo);
    delete cameraMappedBufferManager;
    sceneMappedBufferManager->cleanup(deviceInfo);
    delete sceneMappedBufferManager;
    materialMappedBufferManager->cleanup(deviceInfo);
    delete materialMappedBufferManager;
}

void ModelBufferManager::loadModelIntoBuffer(Device* device, CommandPool* commandPool, RenderModelCreateData modelData)
{
    modelData.meshRenderer->vertexMemChunk = vertexBufferAllocator-> allocateBuffer(device, commandPool, modelData.vertices.data(), sizeof(Vertex) * modelData.vertices.size(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    modelData.meshRenderer->indexMemChunk  = indexBufferAllocator->  allocateBuffer(device, commandPool, modelData.indices.data(), sizeof(uint32_t) * modelData.indices.size(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
}

void ModelBufferManager::bindBuffers(const VkCommandBuffer& commandBuffer)
{
    VkDeviceSize offsets[] = { 0 };

    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBufferAllocator->getBuffer()->getBuffer(), offsets);
    vkCmdBindIndexBuffer(commandBuffer, indexBufferAllocator->getBuffer()->getBuffer(), 0, VK_INDEX_TYPE_UINT32);
}

void ModelBufferManager::updatePushConstants(VkCommandBuffer& commandBuffer, VkPipelineLayout& layout, const PushConstantData& pushConstData)
{
    vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstantData), (void*)&pushConstData);
}

void ModelBufferManager::drawIndexed(const VkCommandBuffer& commandBuffer, const int32_t& indexCount, const int32_t& vertexOffset, const int32_t& indexOffset)
{
    vkCmdDrawIndexed(commandBuffer, indexCount, 1, indexOffset, vertexOffset, 0);
}

void ModelBufferManager::updateUniformBuffer(Device* deviceInfo, const uint32_t& bufferIndex, RenderSceneData& sceneData) {
    // Vertex
    GPUCameraData ubo{};
    ubo.view = sceneData.cameraData.viewMat;
    ubo.proj = sceneData.cameraData.projMat;

    int uboModelIndex = 0;
    for (auto& modelData : sceneData.modelData) {
        for (int i = 0; i < modelData.second.size(); i++, uboModelIndex++) {
            ubo.model[uboModelIndex] = modelData.second[i].modelTransform;
        }
    }

    // Frag
    GPUSceneData scn{};
    int numOfLights = sceneData.lightData.size();
    for (uint32_t i = 0; i < numOfLights; i++) {
        scn.lightInfo[i] = LightInfo(sceneData.lightData[i].lightType, 
            sceneData.lightData[i].lightPosition, 
            sceneData.lightData[i].lightRotation, 
            sceneData.lightData[i].lightColor,
            sceneData.lightData[i].constant,
            sceneData.lightData[i].linear,
            sceneData.lightData[i].quad,
            sceneData.lightData[i].innerCutoff,
            sceneData.lightData[i].outerCutoff);
    }

    scn.ambientColor = glm::vec4(1., 1., 1., 1);
    scn.cameraPosition = glm::vec4(sceneData.cameraData.position, numOfLights);

    memcpy(cameraMappedBufferManager->getMappedBuffer(bufferIndex), &ubo, sizeof(ubo));
    memcpy(sceneMappedBufferManager->getMappedBuffer(bufferIndex), &scn, sizeof(scn));
}

//Index 0 Camera
//Index 1 Scene
//Index 2 Material
MappedBufferManager* ModelBufferManager::getMappedBufferManager(uint32_t index) {
    switch (index) {
    case 0: return cameraMappedBufferManager;
    case 1: return sceneMappedBufferManager;
    case 2: return materialMappedBufferManager;
    default: return nullptr; }
}