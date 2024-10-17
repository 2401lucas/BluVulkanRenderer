#pragma once
#include <vulkan/vulkan_core.h>

#include "VulkanDevice.h"

namespace core_internal::rendering {
class VulkanRenderpass {
 private:
  VkRenderingAttachmentInfoKHR* attachments;
  VkRenderingInfo info;

 public:
  VulkanRenderpass(const core_internal::rendering::Image**, const uint32_t& imgCount);
  ~VulkanRenderpass();
};
}  // namespace core_internal::rendering