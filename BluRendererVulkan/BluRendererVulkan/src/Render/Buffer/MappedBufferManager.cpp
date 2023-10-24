#include "MappedBufferManager.h"

MappedBufferManager::MappedBufferManager(Device* deviceInfo, const uint32_t& numOfBuffers, const VkDeviceSize& size, const VkBufferUsageFlags& usage, const VkMemoryPropertyFlags& properties)
	: BufferManager(deviceInfo, numOfBuffers, size, usage, properties)
{
    mappedBuffers.resize(numOfBuffers);

    for (uint32_t i = 0; i < numOfBuffers; i++) {
        vkMapMemory(deviceInfo->getLogicalDevice(), buffers[i]->getBufferMemory(), 0, size, 0, &mappedBuffers[i]);
    }
}

void* MappedBufferManager::getMappedBuffer(const uint32_t& index)
{
    return mappedBuffers[index];
}
