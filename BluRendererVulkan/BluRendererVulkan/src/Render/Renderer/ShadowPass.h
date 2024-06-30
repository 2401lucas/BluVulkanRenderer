#pragma once

#include <vulkan/vulkan.h>

#include <vector>

#include "../ResourceManagement/VulkanResources/VulkanDevice.h"
#include "OffscreenPass.h"

namespace vks {
class ShadowPass : public OffscreenPass {
 public:
  VulkanDevice* device;

  // TEMP
  std::vector<VkCommandBuffer> cmdBufs;

  ShadowPass(VulkanDevice* device, VkFormat depthFormat, uint32_t imageCount,
             float shadowMapSize, std::string vertexShaderPath);
  ~ShadowPass();
};
}  // namespace vks