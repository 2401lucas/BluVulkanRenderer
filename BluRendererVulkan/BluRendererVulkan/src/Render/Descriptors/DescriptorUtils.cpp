#include "DescriptorUtils.h"

VkDescriptorSetLayoutBinding DescriptorUtils::createDescriptorSetBinding(const uint32_t& binding, const uint32_t& descriptorCount, const VkDescriptorType& descriptorType, const VkSampler* immutableSamplers, const VkShaderStageFlags& shaderStageFlags)
{
    VkDescriptorSetLayoutBinding layoutBinding{};
    layoutBinding.binding = binding;
    layoutBinding.descriptorCount = descriptorCount;
    layoutBinding.descriptorType = descriptorType;
    layoutBinding.pImmutableSamplers = immutableSamplers;
    layoutBinding.stageFlags = shaderStageFlags;
    return layoutBinding;
}

VkWriteDescriptorSet DescriptorUtils::createBufferDescriptorWriteSet(const VkDescriptorSet& dstSet, const uint32_t& dstBinding, const uint32_t& dstArrayElement, const VkDescriptorType& descriptorType, const uint32_t& descriptorCount, const VkDescriptorBufferInfo* bufferInfo)
{
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

VkWriteDescriptorSet DescriptorUtils::createImageDescriptorWriteSet(const VkDescriptorSet& dstSet, const uint32_t& dstBinding, const uint32_t& dstArrayElement, const VkDescriptorType& descriptorType, const uint32_t& descriptorCount, const VkDescriptorImageInfo* imageInfo)
{
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
size_t DescriptorUtils::padUniformBufferSize(const size_t& originalSize, size_t minUniformBufferOffsetAlignment)
{
    // Calculate required alignment based on minimum device offset alignment
    size_t alignedSize = originalSize;
    if (minUniformBufferOffsetAlignment > 0) {
        alignedSize = (alignedSize + minUniformBufferOffsetAlignment - 1) & ~(minUniformBufferOffsetAlignment - 1);
    }

    return alignedSize;
}