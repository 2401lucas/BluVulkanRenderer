#pragma once
#include "BufferManager.h"

class MappedBufferManager : public BufferManager {
public:
	MappedBufferManager(Device* deviceInfo, const uint32_t& numOfBuffers, const VkDeviceSize& size, const VkBufferUsageFlags& usage, const VkMemoryPropertyFlags& properties);

	void updateMappedBuffer(const uint32_t& index, const void* src);
	void updateMappedBufferWithOffset(const uint32_t& index, const void* src, const uint32_t& offset);

protected:
	std::vector<void*> mappedBuffers;
};