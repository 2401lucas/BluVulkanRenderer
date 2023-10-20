#include "DescriptorPool.h"
#include <vulkan/vulkan_core.h>
#include <array>
#include "../include/RenderConst.h"
#include <stdexcept>

DescriptorPool::DescriptorPool(Device* deviceInfo)
{
    std::array<VkDescriptorPoolSize, 2> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(RenderConst::MAX_FRAMES_IN_FLIGHT);
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = static_cast<uint32_t>(RenderConst::MAX_FRAMES_IN_FLIGHT);

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(RenderConst::MAX_FRAMES_IN_FLIGHT);

    if (vkCreateDescriptorPool(deviceInfo->getLogicalDevice(), &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool!");
    }
}

void DescriptorPool::cleanup(Device* deviceInfo)
{
    vkDestroyDescriptorPool(deviceInfo->getLogicalDevice(), descriptorPool, nullptr);
}

VkDescriptorPool DescriptorPool::getDescriptorPool()
{
    return descriptorPool;
}
