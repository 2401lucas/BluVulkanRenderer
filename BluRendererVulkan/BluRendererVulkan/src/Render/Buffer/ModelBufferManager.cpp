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

void ModelBufferManager::createVertexBuffer(Device* deviceInfo, CommandPool* commandPool, std::vector<Vertex> vertices)
{
    VkDeviceSize vertexBufferSize = sizeof(Vertex) * vertices.size();

    Buffer* vertexStagingBuffer = new Buffer(deviceInfo, vertexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    vertexStagingBuffer->copyData(deviceInfo, vertices.data(), 0, vertexBufferSize, 0);

    vertexBuffer = new Buffer(deviceInfo, vertexBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    vertexBuffer->copyBuffer(deviceInfo, commandPool, vertexStagingBuffer, vertexBufferSize);

    vertexStagingBuffer->freeBuffer(deviceInfo);
    delete vertexStagingBuffer;
}

void ModelBufferManager::createIndexBuffer(Device* deviceInfo, CommandPool* commandPool, std::vector<uint32_t> indices)
{
    VkDeviceSize indicesBufferSize = sizeof(uint32_t) * indices.size();

    Buffer* indexStagingBuffer = new Buffer(deviceInfo, indicesBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    indexStagingBuffer->copyData(deviceInfo, indices.data(), 0, indicesBufferSize, 0);

    indexBuffer = new Buffer(deviceInfo, indicesBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    indexBuffer->copyBuffer(deviceInfo, commandPool, indexStagingBuffer, indicesBufferSize);

    indexStagingBuffer->freeBuffer(deviceInfo);
    delete indexStagingBuffer;
}

void ModelBufferManager::bindBuffers(const VkCommandBuffer& commandBuffer, const int32_t index)
{
    VkDeviceSize offsets[] = { 0 };

    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer->getBuffer(), offsets);
    vkCmdBindIndexBuffer(commandBuffer, indexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32);
}

void ModelBufferManager::updatePushConstants(VkCommandBuffer& commandBuffer, VkPipelineLayout& layout, const PushConstantData& pushConstData)
{
    vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstantData), (void*)&pushConstData);
}

void ModelBufferManager::drawIndexed(const VkCommandBuffer& commandBuffer, const uint32_t& index)
{
    vkCmdDrawIndexed(commandBuffer, models[index]->getMesh()->getIndices().size(), 1, 0, 0, 0);
}

void ModelBufferManager::updateUniformBuffer(Device* deviceInfo, Camera* camera, const SceneInfo* sceneInfo, const uint32_t& bufferIndex, std::vector<Model*> models) {
    // Vertex
    GPUCameraData ubo{};
    ubo.view = camera->getViewMat();
    ubo.proj = camera->getProjMat();

    auto modelCount = models.size();

    for (int i = 0; i < modelCount; i++) {
        ubo.model[i] = MathUtils::ApplyTransformAndRotation(models[i]->getPosition(), glm::vec3(0));
    }

    // Frag
    GPUSceneData scn{};
    int numOfLights = sceneInfo->lights.size();
    for (uint32_t i = 0; i < numOfLights; i++) {
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
MappedBufferManager* ModelBufferManager::getMappedBufferManager(uint32_t index) {
    switch (index) {
    case 0: return cameraMappedBufferManager;
    case 1: return sceneMappedBufferManager;
    case 2: return materialMappedBufferManager;
    default: return nullptr; }
}