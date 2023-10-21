#pragma once
#include "../Device/Device.h"
#include "../Command/CommandPool.h"

class Buffer {
public: 
	Buffer(Device* deviceInfo, VkDeviceSize bufferSize, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);

	void copyBuffer(Device* deviceInfo, CommandPool* commandPool, Buffer* srcBuffer, VkDeviceSize size);
	void copyData(Device* deviceInfo, void* src, VkDeviceSize offset, VkDeviceSize deviceSize, VkMemoryMapFlags flags);
	void freeBuffer(Device* deviceInfo);
	VkBuffer& getBuffer();
	VkDeviceMemory getBufferMemory();
	//Copy buffer
	//free buffer
private:
	VkBuffer buffer;
	VkDeviceMemory bufferMemory;
};