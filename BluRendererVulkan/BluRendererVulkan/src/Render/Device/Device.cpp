#include "Device.h"
#include <stdexcept>
#include <map>
#include <set>

const char* deviceExtensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_FRAGMENT_SHADER_BARYCENTRIC_EXTENSION_NAME };

Device::Device(GLFWwindow* window, VulkanInstance* vkInstance, DeviceSettings deviceSettings)
{
	settings = deviceSettings;
	this->window = window;

	if (glfwCreateWindowSurface(vkInstance->get(), window, nullptr, &surface) != VK_SUCCESS)
		throw std::runtime_error("failed to create window surface!");

	// Select a supported render device
	uint32_t deviceCount;
	vkEnumeratePhysicalDevices(vkInstance->get(), &deviceCount, nullptr);

	if (deviceCount == 0) {
		throw std::runtime_error("No physical devices found!");
	}

	std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
	vkEnumeratePhysicalDevices(vkInstance->get(), &deviceCount, physicalDevices.data());
	physicalDevice = ChooseDevice(physicalDevices);
	createLogicalDevice();
}

void Device::cleanup(VulkanInstance* vkInstance) {
	vkDestroyDevice(logicalDevice, nullptr);
	vkDestroySurfaceKHR(vkInstance->get(), surface, nullptr);
	delete queueFamilyIndices;

}

VkPhysicalDevice Device::getPhysicalDevice()
{
	return physicalDevice;
}

VkDevice Device::getLogicalDevice()
{
	return logicalDevice;
}

VkSurfaceKHR& Device::getSurface()
{
	return surface;
}

GLFWwindow* Device::getWindow()
{
	return window;
}

VkSampleCountFlagBits Device::getMaxUsableSampleCount(VkSampleCountFlags counts)
{
	if (counts & VK_SAMPLE_COUNT_64_BIT) {
		return VK_SAMPLE_COUNT_64_BIT;
	}
	if (counts & VK_SAMPLE_COUNT_32_BIT) {
		return VK_SAMPLE_COUNT_32_BIT;
	}
	if (counts & VK_SAMPLE_COUNT_16_BIT) {
		return VK_SAMPLE_COUNT_16_BIT;
	}
	if (counts & VK_SAMPLE_COUNT_8_BIT) {
		return VK_SAMPLE_COUNT_8_BIT;
	}
	if (counts & VK_SAMPLE_COUNT_4_BIT) {
		return VK_SAMPLE_COUNT_4_BIT;
	}
	if (counts & VK_SAMPLE_COUNT_2_BIT) {
		return VK_SAMPLE_COUNT_2_BIT;
	}

	return VK_SAMPLE_COUNT_1_BIT;
}

VkPhysicalDevice Device::ChooseDevice(std::vector<VkPhysicalDevice> deviceList)
{
	std::multimap<int, VkPhysicalDevice> candidates;

	for (auto& device : deviceList)
	{
		int score = rateDeviceSuitability(device);
		candidates.insert(std::make_pair(score, device));
	}

	if (candidates.rbegin()->first > 0) {
		return candidates.rbegin()->second;
	}
	else {
		throw std::runtime_error("failed to find a suitable GPU!");
	}
}

int Device::rateDeviceSuitability(VkPhysicalDevice device)
{
	vkGetPhysicalDeviceProperties(device, &deviceProperties);
	vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

	// Required Features
	if (deviceFeatures.geometryShader != VK_TRUE) {
		return 0;
	}

	int score = 0;

	if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
		score += 10000;
	}
	// Maximum possible size of textures affects graphics quality
	score += deviceProperties.limits.maxImageDimension2D;

	return score;
}

void Device::createLogicalDevice()
{
	QueueFamilyIndices indices = findQueueFamilies();

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

	float queuePriority = 1.0f;
	for (uint32_t queueFamily : uniqueQueueFamilies) {
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkDeviceCreateInfo deviceCreateInfo = {};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
	deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	deviceCreateInfo.pEnabledFeatures = &settings.enabledDeviceFeatures;
	deviceCreateInfo.pNext = &settings.enabledDeviceFeatures12;
	deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions;
	deviceCreateInfo.enabledExtensionCount = sizeof(deviceExtensions) / sizeof(deviceExtensions[0]);

	/*if (Renderer::VkLayerConfig::ARE_VALIDATION_LAYERS_ENABLED) {
		deviceCreateInfo.ppEnabledLayerNames = Renderer::VkLayerConfig::VALIDATION_LAYERS;
		deviceCreateInfo.enabledLayerCount = sizeof(Renderer::VkLayerConfig::VALIDATION_LAYERS) / sizeof(Renderer::VkLayerConfig::VALIDATION_LAYERS[0]);
	}
	else {*/
		deviceCreateInfo.enabledLayerCount = 0;
	//}

	if (vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &logicalDevice) != VK_SUCCESS) {
		throw std::runtime_error("failed to create logical device!");
	}

	vkGetDeviceQueue(logicalDevice, indices.graphicsFamily.value(), 0, &graphicsQueue);
	vkGetDeviceQueue(logicalDevice, indices.presentFamily.value(), 0, &presentQueue);
}

const QueueFamilyIndices Device::findQueueFamilies()
{
	if (queueFamilyIndices != nullptr)
	{
		return *queueFamilyIndices;
	}

	queueFamilyIndices = new QueueFamilyIndices();
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

	for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilies.size()); ++i) {
		const VkQueueFamilyProperties& queueFamily = queueFamilies[i];

		if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			queueFamilyIndices->graphicsFamily = i;
		}

		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);
		if (presentSupport) {
			queueFamilyIndices->presentFamily = i;
		}

		if (queueFamilyIndices->isComplete()) {
			break;
		}
	}

	return *queueFamilyIndices;
}

VkFormat Device::findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
{
	for (VkFormat format : candidates) {
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

		if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
			return format;
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
			return format;
	}

	throw std::runtime_error("failed to find supported format!");
}

uint32_t Device::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}

	throw std::runtime_error("failed to find suitable memory type!");
}

VkSampleCountFlagBits Device::getMipSampleCount()
{
	return settings.aaSamples;
}

VkQueue& Device::getGraphicsQueue()
{
	return graphicsQueue;
}

VkQueue& Device::getPresentQueue()
{
	return presentQueue;
}

const VkPhysicalDeviceProperties Device::getGPUProperties()
{
	return deviceProperties;
}