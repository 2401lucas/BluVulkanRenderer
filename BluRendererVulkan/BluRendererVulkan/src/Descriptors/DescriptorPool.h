#pragma once
#include "../Device/Device.h"

class DescriptorPool {
public:
	DescriptorPool(Device* deviceInfo);
	void cleanup(Device* deviceInfo);

	VkDescriptorPool getDescriptorPool();

private:
	VkDescriptorPool descriptorPool;
};