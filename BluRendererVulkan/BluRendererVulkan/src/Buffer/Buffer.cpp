#include "Buffer.h"
#include <stdexcept>

Buffer::Buffer(Device* deviceInfo, VkDeviceSize bufferSize, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
{
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = bufferSize;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(deviceInfo->getLogicalDevice(), &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to create buffer!");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(deviceInfo->getLogicalDevice(), buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = deviceInfo->findMemoryType(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(deviceInfo->getLogicalDevice(), &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate buffer memory!");
    }

    vkBindBufferMemory(deviceInfo->getLogicalDevice(), buffer, bufferMemory, 0);
}

void Buffer::copyBuffer(Device* deviceInfo, CommandPool* commandPool, Buffer* srcBuffer, VkDeviceSize size)
{
    VkCommandBuffer commandBuffer;
    commandPool->allocCommandBuffer(deviceInfo, commandBuffer);
    commandPool->beginCommandBuffer(commandBuffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    VkBufferCopy copyRegion{};
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer->getBuffer(), buffer, 1, &copyRegion);

    commandPool->endCommandBuffer(commandBuffer);
    commandPool->submitBuffer(deviceInfo->getGraphicsQueue(), commandBuffer);
    commandPool->freeCommandBuffer(deviceInfo, commandBuffer);
}

void Buffer::copyData(Device* deviceInfo, void* src, VkDeviceSize offset, VkDeviceSize deviceSize, VkMemoryMapFlags flags)
{
    void* data;
    vkMapMemory(deviceInfo->getLogicalDevice(), bufferMemory, offset, deviceSize, flags, &data);
    memcpy(data, src, static_cast<size_t>(deviceSize));
    vkUnmapMemory(deviceInfo->getLogicalDevice(), bufferMemory);
}

void Buffer::freeBuffer(Device* deviceInfo)
{
    vkDestroyBuffer(deviceInfo->getLogicalDevice(), buffer, nullptr);
    vkFreeMemory(deviceInfo->getLogicalDevice(), bufferMemory, nullptr);
}

VkBuffer& Buffer::getBuffer()
{
    return buffer;
}

VkDeviceMemory Buffer::getBufferMemory()
{
    return bufferMemory;
}
