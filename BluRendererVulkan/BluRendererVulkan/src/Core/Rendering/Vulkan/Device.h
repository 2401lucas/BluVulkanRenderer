#ifndef VULKANDEVICE_H
#define VULKANDEVICE_H

#include <EASTL/hash_set.h>
#include <EASTL/string.h>
#include <EASTL/vector.h>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

#include "Instance.h"

namespace vk::core {
class Device {
 public:
  Device(const vk::core::Instance* instance,
         const eastl::vector<const char*>& requested_features, void* p_next);
  ~Device();

  VkPhysicalDevice GetPhysicalDevice() const { return physical_device_; }
  VkDevice GetLogicalDevice() const { return device_; }

 private:
  uint32_t GetQueueFamilyIndex(VkQueueFlags queue_flags) const;

  // Vulkan Objects
  VkPhysicalDevice physical_device_ = VK_NULL_HANDLE;
  VkDevice device_;
  VmaAllocator allocator_;

  eastl::hash_set<size_t> supported_extensions_;
  // Vulkan Object Information
  VkPhysicalDeviceProperties physical_device_properties_;
  VkPhysicalDeviceFeatures physical_device_features_;
  VkPhysicalDeviceMemoryProperties physical_device_memory_properties_;
  VkPhysicalDeviceFeatures physical_device_enabled_features_;
  eastl::vector<VkQueueFamilyProperties> queue_family_properties_;
  // Managed Vulkan Resources
  eastl::vector<VkShaderModule> shader_modules_;

  struct {
    uint32_t graphics;
    uint32_t compute;
    uint32_t transfer;
  } queue_family_indicies_;

  struct {
    VkQueue graphics{VK_NULL_HANDLE};
    VkQueue compute{VK_NULL_HANDLE};
    VkQueue transfer{VK_NULL_HANDLE};
  } queues;
};
}  // namespace vk::core

#endif