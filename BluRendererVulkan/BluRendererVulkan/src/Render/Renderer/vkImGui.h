#pragma once

#include <vulkan/vulkan_core.h>

#include <vector>

#include "BaseRenderer.h"

class vkImGUI {
 private:
  VkSampler sampler;
  std::vector<vks::Buffer> vertexBuffers;
  std::vector<vks::Buffer> indexBuffers;
  std::vector<int32_t> vertexCounts;
  std::vector<int32_t> indexCounts;
  VkDeviceMemory fontMemory = VK_NULL_HANDLE;
  VkImage fontImage = VK_NULL_HANDLE;
  VkImageView fontView = VK_NULL_HANDLE;
  VkPipelineCache pipelineCache;
  VkPipelineLayout pipelineLayout;
  VkPipeline pipeline;
  VkDescriptorPool descriptorPool;
  VkDescriptorSetLayout descriptorSetLayout;
  VkDescriptorSet descriptorSet;
  vks::VulkanDevice* device;
  BaseRenderer* baseRenderer;
  ImGuiStyle vulkanStyle;

 public:
  int selectedStyle = 0;
  VkPhysicalDeviceDriverProperties driverProperties = {};

  struct PushConstBlock {
    glm::vec2 scale;
    glm::vec2 translate;
  } pushConstBlock;

  vkImGUI(BaseRenderer* br);
  ~vkImGUI();

  void initResources(BaseRenderer* br,
                     VkQueue copyQueue, const std::string& shadersPath);
  void init(float width, float height);
  void setStyle(uint32_t index);
  void updateBuffers(uint32_t frameIndex);
  void drawFrame(VkCommandBuffer commandBuffer, uint32_t frameIndex);
};
