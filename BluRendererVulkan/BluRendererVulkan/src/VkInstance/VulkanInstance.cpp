#define VK_USE_PLATFORM_WIN32_KHR
#include "VulkanInstance.h"
#include <vector>
#include <stdexcept>
#include "../include/Settings/vkLayerConfig.h"

const char* instanceExtensions[] = { VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME, };


VulkanInstance::VulkanInstance(VkApplicationInfo appInfo)
{
	if (Renderer::VkLayerConfig::ARE_VALIDATION_LAYERS_ENABLED && !checkValidationLayerSupport())
		throw std::runtime_error("validation layers requested, but not available!");

	// TODO: 4 Check that extensions match the extensions required
	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
	std::vector<VkExtensionProperties> extensions(extensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;
	createInfo.enabledExtensionCount = sizeof(instanceExtensions) / sizeof(instanceExtensions[0]);
	createInfo.ppEnabledExtensionNames = instanceExtensions;
	
	if (Renderer::VkLayerConfig::ARE_VALIDATION_LAYERS_ENABLED) {
		createInfo.enabledLayerCount = sizeof(Renderer::VkLayerConfig::VALIDATION_LAYERS) / sizeof(Renderer::VkLayerConfig::VALIDATION_LAYERS[0]);
		createInfo.ppEnabledLayerNames = Renderer::VkLayerConfig::VALIDATION_LAYERS;
	}

	if (vkCreateInstance(&createInfo, nullptr, &vkInstance) != VK_SUCCESS) {
		throw std::runtime_error("Vulkan instance creation failed!");
	}
}

void VulkanInstance::cleanup()
{
	vkDestroyInstance(vkInstance, nullptr);
}

const VkInstance& VulkanInstance::get() const
{
	return vkInstance;
}

bool VulkanInstance::checkValidationLayerSupport()
{
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for (const char* layerName : Renderer::VkLayerConfig::VALIDATION_LAYERS) {
		bool layerFound = false;
		for (const auto& layerProperties : availableLayers) {
			if (strcmp(layerName, layerProperties.layerName) == 0) {
				layerFound = true;
				break;
			}
		}
		if (!layerFound)
			return false;
	}

	return true;
}