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
