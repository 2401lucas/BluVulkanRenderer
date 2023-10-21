#include "UBO.h"

UBO::UBO(Device* deviceInfo, VkDeviceSize deviceSize, uint32_t nBuffers)
{
	buffers.resize(nBuffers);

	for (size_t i = 0; i < nBuffers; i++)
	{
		buffers[i] = new Buffer(deviceInfo, deviceSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	}
}

Buffer* UBO::getBuffer(const uint32_t index)
{
	return buffers[index];
}
