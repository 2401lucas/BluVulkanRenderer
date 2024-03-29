#pragma once

#include <assert.h>
#include <imgui.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vulkan/vulkan.h>

#include <iomanip>
#include <sstream>
#include <vector>

#include "../../Debug/VulkanDebug.h"
#include "../../ResourceManagement/VulkanResources/VulkanBuffer.h"
#include "../../ResourceManagement/VulkanResources/VulkanDevice.h"
#include "../../ResourceManagement/VulkanResources/VulkanTools.h"

namespace vks {
class UIOverlay {
 public:
  vks::VulkanDevice* device;
  VkQueue queue;

  VkSampleCountFlagBits rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  uint32_t subpass = 0;

  vks::Buffer vertexBuffer;
  vks::Buffer indexBuffer;
  int32_t vertexCount = 0;
  int32_t indexCount = 0;

  std::vector<VkPipelineShaderStageCreateInfo> shaders;

  VkDescriptorPool descriptorPool;
  VkDescriptorSetLayout descriptorSetLayout;
  VkDescriptorSet descriptorSet;
  VkPipelineLayout pipelineLayout;
  VkPipeline pipeline;

  VkDeviceMemory fontMemory = VK_NULL_HANDLE;
  VkImage fontImage = VK_NULL_HANDLE;
  VkImageView fontView = VK_NULL_HANDLE;
  VkSampler sampler;

  struct PushConstBlock {
    glm::vec2 scale;
    glm::vec2 translate;
  } pushConstBlock;

  bool visible = true;
  bool updated = false;
  float scale = 1.0f;

  UIOverlay();
  ~UIOverlay();

  void preparePipeline(const VkPipelineCache pipelineCache,
                       const VkRenderPass renderPass,
                       const VkFormat colorFormat, const VkFormat depthFormat);
  void prepareResources();

  bool update();
  void draw(const VkCommandBuffer commandBuffer);
  void resize(uint32_t width, uint32_t height);

  void freeResources();

  bool header(const char* caption);
  bool checkBox(const char* caption, bool* value);
  bool checkBox(const char* caption, int32_t* value);
  bool radioButton(const char* caption, bool value);
  bool inputFloat(const char* caption, float* value, float step,
                  uint32_t precision);
  bool sliderFloat(const char* caption, float* value, float min, float max);
  bool sliderInt(const char* caption, int32_t* value, int32_t min, int32_t max);
  bool comboBox(const char* caption, int32_t* itemindex,
                std::vector<std::string> items);
  bool button(const char* caption);
  bool colorPicker(const char* caption, float* color);
  void text(const char* formatstr, ...);
};
}  // namespace vks