#ifndef VULKANDEVICE_H
#define VULKANDEVICE_H

#define VK_USE_PLATFORM_WIN32_KHR

#include <EASTL/vector.h>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

namespace vk::core {
class Device {
 public:
  Device();
  ~Device();

  VkInstance& getInstance();
  VkPhysicalDevice& getPhysicalDevice();
  VkDevice& getLogicalDevice();

  struct {
    uint32_t graphics;
    uint32_t compute;
    uint32_t transfer;
  } queue_family_indices;

  struct {
    VkQueue graphics{VK_NULL_HANDLE};
    VkQueue compute{VK_NULL_HANDLE};
    VkQueue transfer{VK_NULL_HANDLE};
  } queues;

 private:
  uint32_t getMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties,
                         VkBool32* memTypeFound = nullptr) const;
  uint32_t getQueueFamilyIndex(VkQueueFlags queueFlags) const;
  VkPhysicalDevice choosePhysicalDevice(VkPhysicalDevice* devices,
                                        uint32_t deviceCount);
  int rateDeviceSuitability(VkPhysicalDevice device);
  bool extensionSupported(std::string extension);

  // Vulkan Objects
  VkPhysicalDevice physical_device_;
  VkDevice device_;
  VmaAllocator allocator_;
  // Vulkan Object Information
  VkPhysicalDeviceProperties physical_device_properties_;
  VkPhysicalDeviceFeatures physical_device_features_;
  VkPhysicalDeviceFeatures physical_device_enabled_features_;
  VkPhysicalDeviceMemoryProperties physical_device_memory_properties_;
  eastl::vector<VkQueueFamilyProperties> queue_family_properties_;
  // Managed Vulkan Resources
  eastl::vector<VkShaderModule> shader_modules_;
};
}  // namespace vk::core

#endif