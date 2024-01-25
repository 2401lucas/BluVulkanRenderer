#include "BufferAllocator.h"

BufferAllocator::BufferAllocator(Device* deviceInfo, VkDeviceSize chunkSize, VkDeviceSize allocatedSize, const VkBufferUsageFlags& usage, const VkMemoryPropertyFlags& properties)
	: chunkSize(chunkSize) {
	buffer = new Buffer(deviceInfo, allocatedSize, usage, properties);
}

void BufferAllocator::cleanup(Device* deviceInfo)
{
    buffer->freeBuffer(deviceInfo);
    delete buffer;
}

Buffer* BufferAllocator::getBuffer()
{
    return buffer;
}

MemoryChunk BufferAllocator::allocateBuffer(Device* deviceInfo, CommandPool* commandPool, void* data, VkDeviceSize dataSize, const VkBufferUsageFlags& usage, const VkMemoryPropertyFlags& properties)
{
    if (dataSize > chunkSize)
        throw std::exception("Data is larger than chunk");
    Buffer* stagingBuffer = new Buffer(deviceInfo, dataSize, usage, properties);
    stagingBuffer->copyData(deviceInfo, data, 0, dataSize, 0);
    
    int id;
    if (freeMemoryChunks.empty()) {
        id = memoryChunks.size();
        memoryChunks.push_back(MemoryChunk(id * chunkSize, id));
    }
    else {
        id = freeMemoryChunks.front();
        freeMemoryChunks.pop();
    }

    buffer->copyBuffer(deviceInfo, commandPool, stagingBuffer, dataSize, memoryChunks[id].offset);

    stagingBuffer->freeBuffer(deviceInfo);
    delete stagingBuffer;

    return memoryChunks[id];
}

void BufferAllocator::deleteFromBuffer(MemoryChunk memoryInfo)
{
    freeMemoryChunks.push(memoryInfo.index);
}