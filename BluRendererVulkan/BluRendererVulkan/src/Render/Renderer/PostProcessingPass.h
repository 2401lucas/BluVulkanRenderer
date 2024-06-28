#pragma once

#include <vulkan/vulkan.h>

#include <vector>

#include "../ResourceManagement/VulkanResources/VulkanDevice.h"

namespace vks {
class PostProcessingPass {
 public:
  struct FrameBufferAttachment {
    VkImage image;
    VkDeviceMemory memory;
    VkImageView view;
  };

  struct FrameBuffer {
    VkFramebuffer framebuffer;
    FrameBufferAttachment color, depth;
    VkDescriptorImageInfo descriptor;
  };

  VulkanDevice* device;

  VkPipeline pipeline{VK_NULL_HANDLE};
  VkRenderPass renderPass{VK_NULL_HANDLE};
  VkSampler sampler{VK_NULL_HANDLE};
  std::vector<VkDescriptorSet> descriptorSets;
  std::vector<FrameBuffer> framebuffers;

  std::string vertexShaderPath;
  std::string fragmentShaderPath;

  PostProcessingPass(VulkanDevice* device, VkFormat colorFormat,
                     VkFormat depthFormat, uint32_t imageCount, float width,
                     float height, std::string vertexShaderPath,
                     std::string fragmentShaderPath);
  ~PostProcessingPass();

  // When window has been resized, recreate required resources
  void onResize(uint32_t imageCount, float width, float height);
  void render();

  private:
  VkFormat colorFormat;
  VkFormat depthFormat;

  void cleanResources();
  void createResources(uint32_t imageCount, float width, float height);
};

}  // namespace vks