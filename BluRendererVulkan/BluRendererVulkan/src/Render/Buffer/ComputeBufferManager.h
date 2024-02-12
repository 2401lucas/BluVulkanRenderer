#pragma once
#include "MappedBufferManager.h"

class ComputeBufferManager {
public:
	ComputeBufferManager(Device* deviceInfo);
	void cleanup(Device* deviceInfo);

	MappedBufferManager* getMappedBufferManager(uint32_t index);

private:
	MappedBufferManager* computeBufferManager;

};