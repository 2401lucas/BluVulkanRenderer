#pragma once
#include <vulkan/vulkan_core.h>
#include <vector>

class DescriptorUtils {
public:
	DescriptorUtils() = delete;

	static VkDescriptorSetLayoutBinding createDescriptorSetBinding(const uint32_t& binding, const uint32_t& descriptorCount, const VkDescriptorType& descriptorType, const VkSampler* immutableSamplers, const VkShaderStageFlags& shaderStageFlags);
};