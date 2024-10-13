#pragma once
#include "VulkanDevice.h"

namespace core_internal::rendering {
class VulkanDescriptorSet {
 private:
  VulkanDevice* vulkanDevice;

  std::vector<VkDescriptorSetLayoutBinding> bindings;
  VkDescriptorSetLayout layout = VK_NULL_HANDLE;
  VkDescriptorSet set = VK_NULL_HANDLE;
  VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;

  // Optional
  VkDescriptorPool pool = VK_NULL_HANDLE;

 public:
  explicit VulkanDescriptorSet(core_internal::rendering::VulkanDevice*);
  ~VulkanDescriptorSet();

  operator VkDescriptorSetLayout() { return layout; }
  operator VkDescriptorSet() { return set; }
  operator VkPipelineLayout() { return pipelineLayout; }

  void addBinding(uint32_t binding, VkDescriptorType descriptorType,
                  uint32_t descriptorCount, VkPipelineStageFlags stageFlags,
                  const VkSampler* pImmutableSamplers);
  void initLayout();
  VkDescriptorPool initPool(uint32_t numSets);
  VkPipelineLayout initPipelineLayout(uint32_t numRanges,
                                      const VkPushConstantRange*,
                                      VkPipelineLayoutCreateFlags);
  void allocateDescriptorSets(VkDescriptorPool);

  VkWriteDescriptorSet makeWrite(VkDescriptorSet dstSet, uint32_t dstBinding,
                                 const VkDescriptorImageInfo* pImageInfo,
                                 uint32_t arrayElement = 0) const;
  VkWriteDescriptorSet makeWrite(VkDescriptorSet dstSet, uint32_t dstBinding,
                                 const VkDescriptorBufferInfo* pBufferInfo,
                                 uint32_t arrayElement = 0) const;
  VkWriteDescriptorSet makeWrite(
      VkDescriptorSet dstSet, uint32_t dstBinding,
      const VkWriteDescriptorSetAccelerationStructureKHR* pAccel,
      uint32_t arrayElement = 0) const;
  VkWriteDescriptorSet makeWrite(VkDescriptorSet dstSet, uint32_t dstBinding,
                                 uint32_t arrayElement = 0) const;
};
}  // namespace core_internal::rendering