#pragma once

#include <vector>

#include "VulkanTools.h"
#include "vulkan/vulkan.h"

namespace vks {
class Texture {
 public:
  VkDevice device;
  VkImage image = VK_NULL_HANDLE;
  VkImageLayout imageLayout;
  VkDeviceMemory deviceMemory;
  VkImageView view;
  uint32_t width, height;
  uint32_t mipLevels;
  uint32_t layerCount;
  VkDescriptorImageInfo descriptor;
  VkSampler sampler;

  void updateDescriptor();
  void destroy();
};
}  // namespace vks