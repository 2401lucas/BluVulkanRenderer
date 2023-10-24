#pragma once
#include <vulkan/vulkan_core.h>
#include "../Device/Device.h"
#include "Buffer.h"

class BufferManager {
public:
	BufferManager(Device* deviceInfo, const uint32_t& numOfBuffers, const VkDeviceSize& size, const VkBufferUsageFlags& usage, const VkMemoryPropertyFlags& properties);
	void cleanup(Device* deviceInfo);
	std::vector<Buffer*> getBuffers();
	Buffer* getUniformBuffer(uint32_t index);

protected:
	std::vector<Buffer*> buffers;
};