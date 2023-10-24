#pragma once
#include "../Device/Device.h"

class DescriptorPool {
public:
	DescriptorPool(Device* deviceInfo, const std::vector<VkDescriptorPoolSize>& poolSizes, const int32_t& maxSets, const VkDescriptorPoolCreateFlags& flags);
	void cleanup(Device* deviceInfo);

	VkDescriptorPool getDescriptorPool();

private:
	VkDescriptorPool descriptorPool;
};