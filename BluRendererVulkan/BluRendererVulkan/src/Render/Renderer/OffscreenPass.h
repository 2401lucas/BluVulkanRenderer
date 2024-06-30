#pragma once

#include <vulkan/vulkan.h>

#include <string>
#include <vector>

class OffscreenPass {
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

  VkPipeline pipeline{VK_NULL_HANDLE};
  VkRenderPass renderPass{VK_NULL_HANDLE};
  VkSampler sampler{VK_NULL_HANDLE};
  std::vector<VkDescriptorSet> descriptorSets;
  std::vector<FrameBuffer> framebuffers;

  std::string vertexShaderPath;
  std::string fragmentShaderPath;

  VkFormat colorFormat;
  VkFormat depthFormat;
};