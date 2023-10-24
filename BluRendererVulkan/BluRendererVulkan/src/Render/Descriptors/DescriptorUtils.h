#pragma once
#include <vulkan/vulkan_core.h>
#include <vector>

class DescriptorUtils {
public:
	DescriptorUtils() = delete;

	static VkDescriptorSetLayoutBinding createDescriptorSetBinding(const uint32_t& binding, const uint32_t& descriptorCount, const VkDescriptorType& descriptorType, const VkSampler* immutableSamplers, const VkShaderStageFlags& shaderStageFlags);
	static VkWriteDescriptorSet createBufferDescriptorWriteSet(const VkDescriptorSet& dstSet, const uint32_t& dstBinding, const uint32_t& arrayElement, const VkDescriptorType& descriptorType, const uint32_t& descriptorCount, const VkDescriptorBufferInfo* bufferInfo);
	static VkWriteDescriptorSet createImageDescriptorWriteSet(const VkDescriptorSet& dstSet, const uint32_t& dstBinding, const uint32_t& arrayElement, const VkDescriptorType& descriptorType, const uint32_t& descriptorCount, const VkDescriptorImageInfo* imageInfo);
	static size_t padUniformBufferSize(const size_t& originalSize, size_t minUniformBufferOffsetAlignment);
};