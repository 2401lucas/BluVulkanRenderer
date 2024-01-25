#pragma once
#include "Buffer.h"
#include <queue>

struct MemoryChunk {
	VkDeviceSize index;
	VkDeviceSize offset;

	MemoryChunk() = default;
	MemoryChunk(VkDeviceSize offset, VkDeviceSize index) : offset(offset), index(index) {};
};

class BufferAllocator {
public:
	BufferAllocator(Device* deviceInfo, VkDeviceSize chunkSize, VkDeviceSize allocatedSize, const VkBufferUsageFlags& usage, const VkMemoryPropertyFlags& properties);
	void cleanup(Device* deviceInfo);
	
	Buffer* getBuffer();
	MemoryChunk allocateBuffer(Device* deviceInfo, CommandPool* commandPool, void* data, VkDeviceSize dataSize, const VkBufferUsageFlags& usage, const VkMemoryPropertyFlags& properties);
	void deleteFromBuffer(MemoryChunk memoryInfo);

private:
	Buffer* buffer;
	VkDeviceSize chunkSize;
	std::queue<int> freeMemoryChunks;

	std::vector<MemoryChunk> memoryChunks;
};