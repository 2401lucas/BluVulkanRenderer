#pragma once

#include <vulkan/vulkan.h>

#include "VulkanDevice.h"

namespace vks {
class VulkanRenderTarget {
 public:
  struct ImageAttachment {
    VkImage image;
    VkDeviceMemory memory;
    VkImageView view;
    VkDescriptorImageInfo descriptor;
  };
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

  core_internal::rendering::vulkan::VulkanDevice* device;

  std::string vertexShaderPath;
  std::string fragmentShaderPath;
  VkFormat colorFormat;
  VkFormat depthFormat;

  std::vector<VkCommandBuffer> cmdBufs;

  VkPipeline pipeline{VK_NULL_HANDLE};
  VkPipelineLayout pipelineLayout{VK_NULL_HANDLE};
  VkRenderPass renderPass{VK_NULL_HANDLE};
  VkSampler sampler{VK_NULL_HANDLE};
  VkDescriptorSetLayout descriptorSetLayout{VK_NULL_HANDLE};
  std::vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindings;
  std::vector<VkDescriptorSet> descriptorSets;
  std::vector<VkDescriptorSet> screenTextureDescriptorSets;
  std::vector<FrameBuffer> framebuffers;

  std::vector<Buffer> passBuffers;
  std::vector<ImageAttachment> passImages;

  ~VulkanRenderTarget() {
    for (size_t i = 0; i < framebuffers.size(); i++) {
      vkDestroyImage(device->logicalDevice, framebuffers[i].color.image,
                     nullptr);
      vkDestroyImageView(device->logicalDevice, framebuffers[i].color.view,
                         nullptr);
      vkFreeMemory(device->logicalDevice, framebuffers[i].color.memory,
                   nullptr);

      vkDestroyImage(device->logicalDevice, framebuffers[i].depth.image,
                     nullptr);
      vkDestroyImageView(device->logicalDevice, framebuffers[i].depth.view,
                         nullptr);
      vkFreeMemory(device->logicalDevice, framebuffers[i].depth.memory,
                   nullptr);
      vkDestroyFramebuffer(device->logicalDevice, framebuffers[i].framebuffer,
                           nullptr);
    }

    for (size_t i = 0; i < passBuffers.size(); i++) {
      passBuffers[i].destroy();
    }
    for (size_t i = 0; i < passImages.size(); i++) {
      vkDestroyImage(device->logicalDevice, passImages[i].image, nullptr);
      vkDestroyImageView(device->logicalDevice, passImages[i].view, nullptr);
      vkFreeMemory(device->logicalDevice, passImages[i].memory, nullptr);
    }

    if (pipelineLayout != VK_NULL_HANDLE) {
      vkDestroyPipelineLayout(device->logicalDevice, pipelineLayout, nullptr);
    }
    if (pipeline != VK_NULL_HANDLE) {
      vkDestroyPipeline(device->logicalDevice, pipeline, nullptr);
    }
    if (descriptorSetLayout != VK_NULL_HANDLE) {
      vkDestroyDescriptorSetLayout(device->logicalDevice, descriptorSetLayout,
                                   nullptr);
    }
    if (renderPass != VK_NULL_HANDLE) {
      vkDestroyRenderPass(device->logicalDevice, renderPass, nullptr);
    }
    if (sampler != VK_NULL_HANDLE) {
      vkDestroySampler(device->logicalDevice, sampler, nullptr);
    }
  }
};
}  // namespace vks