#pragma once
#include "BufferManager.h"

class MappedBufferManager : public BufferManager {
public:
	MappedBufferManager(Device* deviceInfo, const uint32_t& numOfBuffers, const VkDeviceSize& size, const VkBufferUsageFlags& usage, const VkMemoryPropertyFlags& properties);

	void* getMappedBuffer(const uint32_t& index);

protected:
	std::vector<void*> mappedBuffers;
};