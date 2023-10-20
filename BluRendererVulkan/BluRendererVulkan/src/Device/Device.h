#pragma once
#include <vector>
#include <vulkan/vulkan_core.h>
#include <optional>
#include <GLFW/glfw3.h>
#include "../VkInstance/VulkanInstance.h"

struct QueueFamilyIndices {
public:
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;

	bool isComplete() {
		return graphicsFamily.has_value() && presentFamily.has_value();
	}
};

struct DeviceSettings {
	VkPhysicalDeviceFeatures enabledDeviceFeatures;
	VkSampleCountFlagBits msaaSamples;
	//VkPhysicalDeviceFeatures deviceFeatures = { };
	//deviceFeatures.samplerAnisotropy = VK_TRUE;
	//deviceFeatures.sampleRateShading = VK_TRUE;
};

class Device {
public:
	Device(GLFWwindow*, VulkanInstance*, DeviceSettings);
	void cleanup(VulkanInstance* vkInstance);

	VkPhysicalDevice getPhysicalDevice();
	VkDevice getLogicalDevice();
	VkSurfaceKHR& getSurface();
	GLFWwindow* getWindow();
	const QueueFamilyIndices findQueueFamilies();
	VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
	uint32_t findMemoryType(uint32_t, VkMemoryPropertyFlags);
	VkSampleCountFlagBits getMipSampleCount();
	VkQueue& getGraphicsQueue();
	VkQueue& getPresentQueue();

private:
	VkSampleCountFlagBits getMaxUsableSampleCount(VkSampleCountFlags);
	VkPhysicalDevice ChooseDevice(std::vector<VkPhysicalDevice>);
	int rateDeviceSuitability(VkPhysicalDevice);
	void createLogicalDevice();

	VkPhysicalDevice physicalDevice;
	VkDevice logicalDevice;
	VkQueue graphicsQueue;
	VkQueue presentQueue;
	VkSurfaceKHR surface;
	GLFWwindow* window;
	DeviceSettings settings;
	QueueFamilyIndices* queueFamilyIndices = nullptr;
};