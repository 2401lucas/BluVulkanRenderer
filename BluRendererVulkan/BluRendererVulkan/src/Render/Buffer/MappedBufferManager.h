#pragma once
#include "BufferManager.h"

class MappedBufferManager : public BufferManager {
public:
	MappedBufferManager(Device* deviceInfo, const uint32_t& numOfBuffers, const VkDeviceSize& size, const VkBufferUsageFlags& usage, const VkMemoryPropertyFlags& properties);

	void updateMappedBuffers(const uint32_t& index, const void* src);

protected:
	std::vector<void*> mappedBuffers;
};