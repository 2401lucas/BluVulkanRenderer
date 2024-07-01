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

  std::string vertexShaderPath;
  std::string fragmentShaderPath;
  VkFormat colorFormat;
  VkFormat depthFormat;

  std::vector<VkCommandBuffer> cmdBufs;

  VkPipeline pipeline{VK_NULL_HANDLE};
  VkPipelineLayout pipelineLayout{VK_NULL_HANDLE};
  VkRenderPass renderPass{VK_NULL_HANDLE};
  VkSampler sampler{VK_NULL_HANDLE};
  std::vector<VkDescriptorSet> screenTextureDescriptorSet;

  
  //Maybe Not needed
  vks::Buffer staticPassBuffer{VK_NULL_HANDLE};
  std::vector<VkDescriptorSetLayoutBinding> staticDescriptorSetLayoutBindings;
  VkDescriptorSetLayout staticDescriptorSetLayout{VK_NULL_HANDLE};
  VkDescriptorSet staticDescriptorSet{VK_NULL_HANDLE};

  std::vector<FrameBuffer> framebuffers;

  PostProcessingPass(VulkanDevice* device, VkFormat colorFormat,
                     VkFormat depthFormat, uint32_t imageCount, float width,
                     float height, std::string vertexShaderPath,
                     std::string fragmentShaderPath);
  virtual ~PostProcessingPass();

  // When window has been resized, recreate required resources
  void onResize(uint32_t imageCount, float width, float height);
  virtual void onRender(VkCommandBuffer curBuf, uint32_t currentFrameIndex);
  virtual void updateDescriptorSets();
  virtual void createUbo();
  virtual void updateUbo();

 private:
  void cleanResources();
  void createResources(uint32_t imageCount, float width, float height);
};
}  // namespace vks