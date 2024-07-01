#pragma once

#include <vulkan/vulkan.h>

#include <vector>

#include "../ResourceManagement/VulkanResources/VulkanDevice.h"

namespace vks {
class ShadowPass {
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

  std::string vertexShaderPath;
  std::string fragmentShaderPath;
  VkFormat colorFormat;
  VkFormat depthFormat;

  std::vector<VkCommandBuffer> cmdBufs;

  std::vector<vks::Buffer> passBuffers;
  VkPipeline pipeline{VK_NULL_HANDLE};
  VkRenderPass renderPass{VK_NULL_HANDLE};
  VkSampler sampler{VK_NULL_HANDLE};
  std::vector<VkDescriptorSet> descriptorSets;
  std::vector<FrameBuffer> framebuffers;

  // TEMP

  ShadowPass(VulkanDevice* device, VkFormat depthFormat, uint32_t imageCount,
             float shadowMapSize, std::string vertexShaderPath);
  ~ShadowPass();
};
}  // namespace vks