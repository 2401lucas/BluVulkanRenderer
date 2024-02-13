#include "ModelBufferManager.h"

#include "../Math/MathUtils.h"

ModelBufferManager::ModelBufferManager(Device* deviceInfo) {
  cameraMappedBufferManager = new MappedBufferManager(
      deviceInfo, RenderConst::MAX_FRAMES_IN_FLIGHT, sizeof(GPUCameraData),
      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  sceneMappedBufferManager = new MappedBufferManager(
      deviceInfo, RenderConst::MAX_FRAMES_IN_FLIGHT, sizeof(GPUSceneData),
      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  std::vector<VkDescriptorPoolSize> sceneInfoPoolSizes{2,
                                                       VkDescriptorPoolSize()};
  sceneInfoPoolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  sceneInfoPoolSizes[0].descriptorCount =
      static_cast<uint32_t>(RenderConst::MAX_FRAMES_IN_FLIGHT);
  sceneInfoPoolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  sceneInfoPoolSizes[1].descriptorCount =
      static_cast<uint32_t>(RenderConst::MAX_FRAMES_IN_FLIGHT);
  std::vector<VkDescriptorPoolSize> texturePoolSizes{2, VkDescriptorPoolSize()};
  texturePoolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  texturePoolSizes[0].descriptorCount =
      static_cast<uint32_t>(RenderConst::MAX_FRAMES_IN_FLIGHT);
  texturePoolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  texturePoolSizes[1].descriptorCount =
      static_cast<uint32_t>(RenderConst::MAX_FRAMES_IN_FLIGHT);

  globalInfoDescriptorPool = new DescriptorPool(
      deviceInfo, sceneInfoPoolSizes, RenderConst::MAX_FRAMES_IN_FLIGHT, 0);
  textureDescriptorPool = new DescriptorPool(
      deviceInfo, texturePoolSizes, RenderConst::MAX_FRAMES_IN_FLIGHT, 0);
}

void ModelBufferManager::cleanup(Device* deviceInfo) {
  globalInfoDescriptorPool->cleanup(deviceInfo);
  delete globalInfoDescriptorPool;
  textureDescriptorPool->cleanup(deviceInfo);
  delete textureDescriptorPool;
  cameraMappedBufferManager->cleanup(deviceInfo);
  delete cameraMappedBufferManager;
  sceneMappedBufferManager->cleanup(deviceInfo);
  delete sceneMappedBufferManager;
}

void ModelBufferManager::updatePushConstants(
    VkCommandBuffer& commandBuffer, VkPipelineLayout& layout,
    const PushConstantData& pushConstData) {
  vkCmdPushConstants(commandBuffer, layout,
                     VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                     0, sizeof(PushConstantData), (void*)&pushConstData);
}

void ModelBufferManager::generateDescriptorSets(
    Device* device, std::vector<VkDescriptorSetLayout>& descriptorLayouts,
    std::vector<TextureData> textureData) {
  DescriptorUtils::allocateDesriptorSets(device, descriptorLayouts[0],
                                         globalInfoDescriptorPool,
                                         globalDescriptorSets);

  std::vector<VkDescriptorBufferInfo> sceneBufferInfos;
  for (size_t i = 0; i < RenderConst::MAX_FRAMES_IN_FLIGHT; i++) {
    VkDescriptorBufferInfo gpuSceneBufferInfo{};
    gpuSceneBufferInfo.buffer =
        sceneMappedBufferManager->getUniformBuffer(i)->getBuffer();
    gpuSceneBufferInfo.offset = 0;
    gpuSceneBufferInfo.range = sizeof(GPUSceneData);
    sceneBufferInfos.push_back(gpuSceneBufferInfo);
  }
  DescriptorUtils::createBufferDescriptorSet(device, globalDescriptorSets, 0,
                                             sceneBufferInfos);

  DescriptorUtils::allocateDesriptorSets(device, descriptorLayouts[1],
                                         textureDescriptorPool,
                                         textureDescriptorSets);
  std::vector<std::vector<VkDescriptorImageInfo>> textureImageDescriptorInfos;
  for (size_t i = 0; i < RenderConst::MAX_FRAMES_IN_FLIGHT; i++) {
    std::vector<VkDescriptorImageInfo> textureImageDescriptorInfo;
    for (uint32_t texIndex = 0; texIndex < textureData.size(); texIndex++) {
      VkDescriptorImageInfo texImageInfo{};
      texImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      texImageInfo.imageView = textureData[texIndex].texture->getImageView();
      texImageInfo.sampler = textureData[texIndex].texture->getImageSampler();
      textureImageDescriptorInfo.push_back(texImageInfo);
      VkDescriptorImageInfo diffImageInfo{};
      diffImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      diffImageInfo.imageView = textureData[texIndex].diffuse->getImageView();
      diffImageInfo.sampler = textureData[texIndex].diffuse->getImageSampler();
      textureImageDescriptorInfo.push_back(diffImageInfo);
      VkDescriptorImageInfo specImageInfo{};
      specImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      specImageInfo.imageView = textureData[texIndex].specular->getImageView();
      specImageInfo.sampler = textureData[texIndex].specular->getImageSampler();
      textureImageDescriptorInfo.push_back(specImageInfo);
    }
    textureImageDescriptorInfos.push_back(textureImageDescriptorInfo);
  }
  DescriptorUtils::createImageDescriptorSet(device, textureDescriptorSets,
                                            textureImageDescriptorInfos);
}

std::vector<InstanceData> ModelBufferManager::updateUniformBuffer(
    Device* deviceInfo, const uint32_t& bufferIndex,
    RenderSceneData& sceneData) {
  auto vp = sceneData.cameraData.projMat * sceneData.cameraData.viewMat;

  std::vector<InstanceData> modelDatas;
  for (auto& modelData : sceneData.modelData) {
    for (int i = 0; i < modelData.second.size(); i++) {
      InstanceData data;
      data.mvp = vp * modelData.second[i].modelTransform;
      data.texIndex = modelData.second[i].materialData.y;
      modelDatas.push_back(data);
    }
  }

  // Frag
  GPUSceneData scn{};
  int numOfLights = sceneData.lightData.size();
  for (uint32_t i = 0; i < numOfLights; i++) {
    scn.lightInfo[i] = LightInfo(
        sceneData.lightData[i].lightType, sceneData.lightData[i].lightPosition,
        sceneData.lightData[i].lightRotation, sceneData.lightData[i].lightColor,
        sceneData.lightData[i].constant, sceneData.lightData[i].linear,
        sceneData.lightData[i].quad, sceneData.lightData[i].innerCutoff,
        sceneData.lightData[i].outerCutoff);
  }

  scn.ambientColor = glm::vec4(1., 1., 1., 1);
  scn.cameraPosition = glm::vec4(sceneData.cameraData.position, numOfLights);

  memcpy(sceneMappedBufferManager->getMappedBuffer(bufferIndex), &scn,
         sizeof(scn));
  return modelDatas;
}

// Index 0 Camera
// Index 1 Scene
MappedBufferManager* ModelBufferManager::getMappedBufferManager(
    uint32_t index) {
  switch (index) {
    case 0:
      return cameraMappedBufferManager;
    case 1:
      return sceneMappedBufferManager;
    default:
      return nullptr;
  }
}

VkDescriptorSet* ModelBufferManager::getGlobalDescriptorSet(
    uint32_t frameIndex) {
  return &globalDescriptorSets[frameIndex];
}

VkDescriptorSet* ModelBufferManager::getTextureDescriptorSet(
    uint32_t frameIndex) {
  return &textureDescriptorSets[frameIndex];
}
