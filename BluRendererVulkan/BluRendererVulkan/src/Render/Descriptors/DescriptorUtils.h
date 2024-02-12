#pragma once
#include <vulkan/vulkan_core.h>

#include <vector>

#include "../Textures/TextureManager.h"
#include "DescriptorPool.h"

class DescriptorUtils {
 public:
  DescriptorUtils() = delete;

  static void allocateDesriptorSets(
      Device* deviceInfo, VkDescriptorSetLayout descriptorSetLayout,
      DescriptorPool* pool, std::vector<VkDescriptorSet>& descriptorSets);
  static void createBufferDescriptorSet(
      Device* deviceInfo, std::vector<VkDescriptorSet>& descriptorSets,
      uint32_t binding, std::vector<VkDescriptorBufferInfo> descriptorSetInfo);
  static void createImageDescriptorSet(
      Device* deviceInfo, std::vector<VkDescriptorSet>& descriptorSets,
      std::vector<std::vector<VkDescriptorImageInfo>> descriptorSetInfo);
  static VkDescriptorSetLayoutBinding createDescriptorSetBinding(
      const uint32_t& binding, const uint32_t& descriptorCount,
      const VkDescriptorType& descriptorType,
      const VkSampler* immutableSamplers,
      const VkShaderStageFlags& shaderStageFlags);
  static VkWriteDescriptorSet createBufferDescriptorWriteSet(
      const VkDescriptorSet& dstSet, const uint32_t& dstBinding,
      const uint32_t& arrayElement, const VkDescriptorType& descriptorType,
      const uint32_t& descriptorCount,
      const VkDescriptorBufferInfo* bufferInfo);
  static VkWriteDescriptorSet createImageDescriptorWriteSet(
      const VkDescriptorSet& dstSet, const uint32_t& dstBinding,
      const uint32_t& arrayElement, const VkDescriptorType& descriptorType,
      const uint32_t& descriptorCount, const VkDescriptorImageInfo* imageInfo);
  static size_t padUniformBufferSize(const size_t& originalSize,
                                     size_t minUniformBufferOffsetAlignment);
};