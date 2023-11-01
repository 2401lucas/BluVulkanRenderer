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
	VkPhysicalDeviceVulkan12Features enabledDeviceFeatures12;
	VkPhysicalDeviceFragmentShaderBarycentricFeaturesKHR enabledFragShaderBarycentricFeatures;
	VkSampleCountFlagBits msaaSamples;
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
	const VkPhysicalDeviceProperties getGPUProperties();

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
	VkPhysicalDeviceProperties deviceProperties;
	VkPhysicalDeviceFeatures deviceFeatures;
};