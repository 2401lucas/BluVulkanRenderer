#pragma once
#include <vulkan/vulkan.h>

class VulkanInstance {
public:
	VulkanInstance(VkApplicationInfo);
	void cleanup();

	const VkInstance& get() const;

private:
	bool checkValidationLayerSupport();

	VkInstance vkInstance;
};