#pragma once

#include <imgui.h>

#include <array>
#include <glm/ext/vector_float2.hpp>

#include "../Buffer/Buffer.h"
#include "../Descriptors/Descriptor.h"
#include "../Descriptors/DescriptorPool.h"
#include "../Image/Image.h"
#include "../Pipeline/GraphicsPipeline.h"

class UI {
 public:
  UI(Device*, CommandPool*, RenderPass*);
  void cleanup(Device*);
  void newFrame(Device*, bool updateFrameGraph);
  void updateBuffers(Device*);
  void draw(VkCommandBuffer, int frameNum);

 private:
  void setStyle(uint32_t index);
  struct UISettings {
    bool displayModels = true;
    bool displayLogos = true;
    bool displayBackground = true;
    bool animateLight = false;
    float lightSpeed = 0.25f;
    std::array<float, 50> frameTimes{};
    float frameTimeMin = 9999.0f, frameTimeMax = 0.0f;
    float lightTimer = 0.0f;
  } uiSettings;

  DescriptorPool* uiDescriptorPool;
  Descriptor* uiDescriptorSetLayout;
  std::vector<VkDescriptorSet> uiDescriptorSets;
  GraphicsPipeline* uiPipeline;
  Buffer* vertexBuffer = nullptr;
  Buffer* indexBuffer = nullptr;
  int32_t vertexCount = 0;
  int32_t indexCount = 0;
  Image* uiImage;
  ImGuiStyle vulkanStyle;
  int selectedStyle = 0;

 public:
  // UI params are set via push constants
  struct PushConstBlock {
    glm::vec2 scale;
    glm::vec2 translate;
  };
};