#include "DescriptorUtils.h"

#include <stdexcept>

#include "../../../include/RenderConst.h"

void DescriptorUtils::allocateDesriptorSets(
    Device* deviceInfo, VkDescriptorSetLayout descriptorSetLayout,
    DescriptorPool* pool, std::vector<VkDescriptorSet>& descriptorSets) {
  std::vector<VkDescriptorSetLayout> descriptorLayout(
      RenderConst::MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
  VkDescriptorSetAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = pool->getDescriptorPool();
  allocInfo.descriptorSetCount =
      static_cast<uint32_t>(RenderConst::MAX_FRAMES_IN_FLIGHT);
  allocInfo.pSetLayouts = descriptorLayout.data();

  descriptorSets.resize(RenderConst::MAX_FRAMES_IN_FLIGHT);
  if (vkAllocateDescriptorSets(deviceInfo->getLogicalDevice(), &allocInfo,
                               descriptorSets.data()) != VK_SUCCESS) {
    throw std::runtime_error("failed to allocate descriptor sets!");
  }
}

void DescriptorUtils::createBufferDescriptorSet(
    Device* deviceInfo, std::vector<VkDescriptorSet>& descriptorSets,
    uint32_t binding, std::vector<VkDescriptorBufferInfo> descriptorSetInfo) {
  for (uint32_t i = 0; i < RenderConst::MAX_FRAMES_IN_FLIGHT; i++) {
    std::vector<VkWriteDescriptorSet> descriptorWrites;

    descriptorWrites.push_back(DescriptorUtils::createBufferDescriptorWriteSet(
        descriptorSets[i], binding, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1,
        &descriptorSetInfo[i]));

    vkUpdateDescriptorSets(deviceInfo->getLogicalDevice(),
                           static_cast<uint32_t>(descriptorWrites.size()),
                           descriptorWrites.data(), 0, nullptr);
  }
}

void DescriptorUtils::createImageDescriptorSet(
    Device* deviceInfo, std::vector<VkDescriptorSet>& descriptorSets,
    std::vector<std::vector<VkDescriptorImageInfo>> descriptorSetInfo) {
  for (uint32_t i = 0; i < RenderConst::MAX_FRAMES_IN_FLIGHT; i++) {
    std::vector<VkWriteDescriptorSet> descriptorWrites;

    descriptorWrites.push_back(DescriptorUtils::createImageDescriptorWriteSet(
        descriptorSets[i], 0, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        descriptorSetInfo[i].size(), descriptorSetInfo[i].data()));

    vkUpdateDescriptorSets(deviceInfo->getLogicalDevice(),
                           static_cast<uint32_t>(descriptorWrites.size()),
                           descriptorWrites.data(), 0, nullptr);
  }
}

VkDescriptorSetLayoutBinding DescriptorUtils::createDescriptorSetBinding(
    const uint32_t& binding, const uint32_t& descriptorCount,
    const VkDescriptorType& descriptorType, const VkSampler* immutableSamplers,
    const VkShaderStageFlags& shaderStageFlags) {
  VkDescriptorSetLayoutBinding layoutBinding{};
  layoutBinding.binding = binding;
  layoutBinding.descriptorCount = descriptorCount;
  layoutBinding.descriptorType = descriptorType;
  layoutBinding.pImmutableSamplers = immutableSamplers;
  layoutBinding.stageFlags = shaderStageFlags;
  return layoutBinding;
}

VkWriteDescriptorSet DescriptorUtils::createBufferDescriptorWriteSet(
    const VkDescriptorSet& dstSet, const uint32_t& dstBinding,
    const uint32_t& dstArrayElement, const VkDescriptorType& descriptorType,
    const uint32_t& descriptorCount, const VkDescriptorBufferInfo* bufferInfo) {
  VkWriteDescriptorSet descriptorWrite{};
  descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrite.dstSet = dstSet;
  descriptorWrite.dstBinding = dstBinding;
  descriptorWrite.dstArrayElement = dstArrayElement;
  descriptorWrite.descriptorType = descriptorType;
  descriptorWrite.descriptorCount = descriptorCount;
  descriptorWrite.pBufferInfo = bufferInfo;

  return descriptorWrite;
}

VkWriteDescriptorSet DescriptorUtils::createImageDescriptorWriteSet(
    const VkDescriptorSet& dstSet, const uint32_t& dstBinding,
    const uint32_t& dstArrayElement, const VkDescriptorType& descriptorType,
    const uint32_t& descriptorCount, const VkDescriptorImageInfo* imageInfo) {
  VkWriteDescriptorSet descriptorWrite{};
  descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrite.dstSet = dstSet;
  descriptorWrite.dstBinding = dstBinding;
  descriptorWrite.dstArrayElement = dstArrayElement;
  descriptorWrite.descriptorType = descriptorType;
  descriptorWrite.descriptorCount = descriptorCount;
  descriptorWrite.pImageInfo = imageInfo;

  return descriptorWrite;
}

// Courtesy of Sascha Willems
// https://github.com/SaschaWillems/Vulkan/tree/master/examples/dynamicuniformbuffer
size_t DescriptorUtils::padUniformBufferSize(
    const size_t& originalSize, size_t minUniformBufferOffsetAlignment) {
  // Calculate required alignment based on minimum device offset alignment
  size_t alignedSize = originalSize;
  if (minUniformBufferOffsetAlignment > 0) {
    alignedSize = (alignedSize + minUniformBufferOffsetAlignment - 1) &
                  ~(minUniformBufferOffsetAlignment - 1);
  }

  return alignedSize;
}