#pragma once

#include <vector>

#include "VulkanTools.h"
#include "vulkan/vulkan.h"

namespace vks {
struct Buffer {
 public:
  std::vector<VkBuffer> buffer;
  std::vector<VkDeviceMemory> memory;
  std::vector<VkDescriptorBufferInfo> descriptor;
  VkDeviceSize alignment = 0;
  VkDeviceSize size = 0;
  void* mapped = nullptr;
  VkBufferUsageFlags usageFlags;
  VkMemoryPropertyFlags memoryPropertyFlags;
};
}  // namespace vks