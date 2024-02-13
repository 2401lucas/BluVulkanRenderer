#pragma once
#include <vector>

#include "../../Engine/Mesh/MeshUtils.h"
#include "../Buffer/Buffer.h"
#include "../Buffer/MappedBufferManager.h"
#include "../Command/CommandPool.h"
#include "../Descriptors/DescriptorPool.h"
#include "../Descriptors/DescriptorUtils.h"
#include "../Descriptors/Types/PushConsts/PushConst.h"
#include "../Descriptors/Types/UBO/UBO.h"
#include "../Image/Image.h"
#include "../Renderer/RenderSceneData.h"
#include "BufferAllocator.h"

class ModelBufferManager {
 public:
  ModelBufferManager(Device* deviceInfo);
  void cleanup(Device* deviceInfo);

  void generateDescriptorSets(
      Device* device, std::vector<VkDescriptorSetLayout>& descriptorLayouts,
      std::vector<TextureData> textureData);
  std::vector<InstanceData> updateUniformBuffer(Device* deviceInfo,
                                                const uint32_t& bufferIndex,
                           RenderSceneData& sceneData);
  void updatePushConstants(VkCommandBuffer& commandBuffer,
                           VkPipelineLayout& layout,
                           const PushConstantData& pushConstData);
  MappedBufferManager* getMappedBufferManager(uint32_t index);
  VkDescriptorSet* getGlobalDescriptorSet(uint32_t frameIndex);
  VkDescriptorSet* getTextureDescriptorSet(uint32_t frameIndex);

 private:
  MappedBufferManager* cameraMappedBufferManager;
  MappedBufferManager* sceneMappedBufferManager;
  std::vector<VkDescriptorSet> globalDescriptorSets;
  std::vector<VkDescriptorSet> textureDescriptorSets;

  DescriptorPool* globalInfoDescriptorPool;
  DescriptorPool* textureDescriptorPool;
};