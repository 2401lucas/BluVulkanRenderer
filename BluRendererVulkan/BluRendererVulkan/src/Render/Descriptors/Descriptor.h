#pragma once

#include <vector>
#include <vulkan/vulkan_core.h>
#include "../Device/Device.h"

class Descriptor {
public:
	Descriptor(Device* deviceInfo, std::vector<VkDescriptorSetLayoutBinding> bindings);
	void cleanup(Device* deviceInfo);

	const VkDescriptorSetLayout* getLayout();

private:
	VkDescriptorSetLayout descriptorSetLayout;
};