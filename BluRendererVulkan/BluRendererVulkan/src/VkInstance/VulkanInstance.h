#pragma once
#include <vulkan/vulkan.h>

class VulkanInstance {
public:
	VulkanInstance(VkApplicationInfo);
	~VulkanInstance();

	const VkInstance& get() const;

private:
	bool checkValidationLayerSupport();

	VkInstance vkInstance;
};