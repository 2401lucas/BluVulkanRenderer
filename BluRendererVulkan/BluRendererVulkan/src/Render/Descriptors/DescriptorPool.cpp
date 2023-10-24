#include "DescriptorPool.h"
#include <vulkan/vulkan_core.h>
#include <array>
#include "../include/RenderConst.h"
#include <stdexcept>

DescriptorPool::DescriptorPool(Device* deviceInfo, const std::vector<VkDescriptorPoolSize>& poolSizes, const int32_t& maxSets, const VkDescriptorPoolCreateFlags& flags)
{
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(RenderConst::MAX_FRAMES_IN_FLIGHT);
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

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
