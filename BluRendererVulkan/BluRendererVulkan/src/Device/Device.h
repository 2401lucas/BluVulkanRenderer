#include <vector>
#include <vulkan/vulkan_core.h>
#include <optional>

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
	VkPhysicalDeviceFeatures requiredDeviceFeatures;
	//VkPhysicalDeviceFeatures deviceFeatures = { };
	//deviceFeatures.samplerAnisotropy = VK_TRUE;
	//deviceFeatures.sampleRateShading = VK_TRUE;
};

class Device {
public:
	Device(VkInstance, DeviceSettings, VkSurfaceKHR);
	~Device();

	VkPhysicalDevice GetPhysicalDevice();
	VkDevice GetLogicalDevice();
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice, VkSurfaceKHR);

private:
	VkSampleCountFlagBits getMaxUsableSampleCount();
	VkPhysicalDevice ChooseDevice(std::vector<VkPhysicalDevice>);
	int rateDeviceSuitability(VkPhysicalDevice);
	bool isDeviceSuitable(VkPhysicalDevice);
	void CreateLogicalDevice(VkSurfaceKHR);


	DeviceSettings settings;
	VkSampleCountFlagBits msaaSamples;
	VkPhysicalDevice physicalDevice;
	VkDevice logicalDevice;
};