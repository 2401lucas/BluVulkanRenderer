#include "MappedBufferManager.h"

MappedBufferManager::MappedBufferManager(Device* deviceInfo, const uint32_t& numOfBuffers, const VkDeviceSize& size, const VkBufferUsageFlags& usage, const VkMemoryPropertyFlags& properties)
	: BufferManager(deviceInfo, numOfBuffers, size, usage, properties)
{
    mappedBuffers.resize(numOfBuffers);

    for (size_t i = 0; i < numOfBuffers; i++) {
        vkMapMemory(deviceInfo->getLogicalDevice(), buffers[i]->getBufferMemory(), 0, size, 0, &mappedBuffers[i]);
    }
}

void MappedBufferManager::updateMappedBuffer(const uint32_t& index, const void* src)
{
    memcpy(mappedBuffers[index], &src, sizeof(src));
}

void MappedBufferManager::updateMappedBufferWithOffset(const uint32_t& index, const void* src, const uint32_t& offset)
{
    memcpy(&mappedBuffers[index] + offset, &src, sizeof(src));
}