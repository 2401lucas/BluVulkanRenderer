#pragma once

#include <vulkan/vulkan.h>

#include <vector>

#include "../ResourceManagement/VulkanResources/VulkanDevice.h"
#include "OffscreenPass.h"

namespace vks {
class PostProcessingPass : public OffscreenPass {
 public:
  VulkanDevice* device;

  // TEMP
  std::vector<VkCommandBuffer> cmdBufs;

  PostProcessingPass(VulkanDevice* device, VkFormat colorFormat,
                     VkFormat depthFormat, uint32_t imageCount, float width,
                     float height, std::string vertexShaderPath,
                     std::string fragmentShaderPath);
  ~PostProcessingPass();

  // When window has been resized, recreate required resources
  void onResize(uint32_t imageCount, float width, float height);

 private:
  void cleanResources();
  void createResources(uint32_t imageCount, float width, float height);
};
}  // namespace vks