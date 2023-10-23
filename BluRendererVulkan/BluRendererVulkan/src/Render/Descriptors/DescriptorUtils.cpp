#include "DescriptorUtils.h"

VkDescriptorSetLayoutBinding DescriptorUtils::createDescriptorSetBinding(const uint32_t& binding, const uint32_t& descriptorCount, const VkDescriptorType& descriptorType, const VkSampler& immutableSamplers, const VkShaderStageFlags& shaderStageFlags)
{
    VkDescriptorSetLayoutBinding layoutBinding{};
    layoutBinding.binding = 0;
    layoutBinding.descriptorCount = 1;
    layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    layoutBinding.pImmutableSamplers = nullptr;
    layoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

}
