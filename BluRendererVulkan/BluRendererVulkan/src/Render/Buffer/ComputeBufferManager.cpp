#include "ComputeBufferManager.h"
#include "../../../include/RenderConst.h"

ComputeBufferManager::ComputeBufferManager(Device* deviceInfo)
{
	computeBufferManager = new MappedBufferManager(deviceInfo, RenderConst::MAX_FRAMES_IN_FLIGHT, sizeof(0), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
}

void ComputeBufferManager::cleanup(Device* deviceInfo)
{
	computeBufferManager->cleanup(deviceInfo);
	delete computeBufferManager;
}

MappedBufferManager* ComputeBufferManager::getMappedBufferManager(uint32_t index)
{
	switch (index) {
	case 0: return computeBufferManager;
	default: return nullptr; }
}
