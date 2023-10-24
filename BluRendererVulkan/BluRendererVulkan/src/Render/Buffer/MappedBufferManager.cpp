#include "MappedBufferManager.h"
#include "../../../include/RenderConst.h"

MappedBufferManager::MappedBufferManager(Device* deviceInfo, const uint32_t& numOfBuffers, const VkDeviceSize& size, const VkBufferUsageFlags& usage, const VkMemoryPropertyFlags& properties)
	: BufferManager(deviceInfo, numOfBuffers, size, usage, properties)
{
    mappedBuffers.resize(numOfBuffers);

    for (size_t i = 0; i < numOfBuffers; i++) {
        vkMapMemory(deviceInfo->getLogicalDevice(), buffers[i]->getBufferMemory(), 0, size, 0, &mappedBuffers[i]);
    }
}

void MappedBufferManager::updateMappedBuffers(const uint32_t& index, const void* src)
{
    memcpy(mappedBuffers[index], &src, sizeof(src));
}
