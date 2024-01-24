#include "ModelBufferManager.h"
#include "../Math/MathUtils.h"

ModelBufferManager::ModelBufferManager(Device* deviceInfo)
{
    cameraMappedBufferManager = new MappedBufferManager(deviceInfo, RenderConst::MAX_FRAMES_IN_FLIGHT, sizeof(GPUCameraData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    sceneMappedBufferManager = new MappedBufferManager(deviceInfo, RenderConst::MAX_FRAMES_IN_FLIGHT, sizeof(GPUSceneData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    materialMappedBufferManager = new MappedBufferManager(deviceInfo, RenderConst::MAX_FRAMES_IN_FLIGHT, sizeof(GPUMaterialData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
}

void ModelBufferManager::cleanup(Device* deviceInfo)
{
    vertexBuffer->freeBuffer(deviceInfo);
    delete vertexBuffer;

    indexBuffer->freeBuffer(deviceInfo);
    delete indexBuffer;

    cameraMappedBufferManager->cleanup(deviceInfo);
    delete cameraMappedBufferManager;
    sceneMappedBufferManager->cleanup(deviceInfo);
    delete sceneMappedBufferManager;
    materialMappedBufferManager->cleanup(deviceInfo);
    delete materialMappedBufferManager;
}

//This leaks memory bad, shouldn't be being recreated every frame
void ModelBufferManager::prepareBuffer(Device* deviceInfo, CommandPool* commandPool, std::vector<Vertex> vertices, std::vector<uint32_t> indices)
{
    VkDeviceSize vertexBufferSize = sizeof(Vertex) * vertices.size();
    Buffer* vertexStagingBuffer = new Buffer(deviceInfo, vertexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    vertexStagingBuffer->copyData(deviceInfo, vertices.data(), 0, vertexBufferSize, 0);

    vertexBuffer = new Buffer(deviceInfo, vertexBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    vertexBuffer->copyBuffer(deviceInfo, commandPool, vertexStagingBuffer, vertexBufferSize);

    vertexStagingBuffer->freeBuffer(deviceInfo);
    delete vertexStagingBuffer;

    VkDeviceSize indicesBufferSize = sizeof(uint32_t) * indices.size();
    Buffer* indexStagingBuffer = new Buffer(deviceInfo, indicesBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    indexStagingBuffer->copyData(deviceInfo, indices.data(), 0, indicesBufferSize, 0);

    indexBuffer = new Buffer(deviceInfo, indicesBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    indexBuffer->copyBuffer(deviceInfo, commandPool, indexStagingBuffer, indicesBufferSize);

    indexStagingBuffer->freeBuffer(deviceInfo);
    delete indexStagingBuffer;
}

void ModelBufferManager::bindBuffers(const VkCommandBuffer& commandBuffer)
{
    VkDeviceSize offsets[] = { 0 };

    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer->getBuffer(), offsets);
    vkCmdBindIndexBuffer(commandBuffer, indexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32);
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
    scn.ambientColor = glm::vec4(0.1,0.1,0.1,0.1);
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