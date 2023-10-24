#include "BufferManager.h"

BufferManager::BufferManager()
{
}

BufferManager::BufferManager(Device* deviceInfo, const uint32_t& numOfBuffers, const VkDeviceSize& size, const VkBufferUsageFlags& usage, const VkMemoryPropertyFlags& properties)
{
	for (uint32_t i = 0; i < numOfBuffers; i++)
	{
		buffers.push_back(new Buffer(deviceInfo, size, usage, properties));
	}
}

void BufferManager::cleanup(Device* deviceInfo)
{
	for (auto& buffer : buffers)
	{
		buffer->freeBuffer(deviceInfo);
		delete buffer;
	}
}

std::vector<Buffer*> BufferManager::getBuffers()
{
	return buffers;
}

Buffer* BufferManager::getUniformBuffer(uint32_t index)
{
	return buffers[index];
}