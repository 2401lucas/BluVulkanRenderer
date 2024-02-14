#pragma once
#include "../../Engine/Scene/Scene.h"
#include "../Buffer/ModelBufferManager.h"
#include "../Descriptors/Descriptor.h"
#include "../Descriptors/DescriptorPool.h"
#include "../ImGUI/UI.h"
#include "../Pipeline/ComputePipeline.h"
#include "../Pipeline/GraphicsPipeline.h"
#include "../RenderPass/RenderPass.h"
#include "BaseRenderer.h"

class DefaultRenderer : public BaseRenderer {
 public:
  DefaultRenderer(GLFWwindow* window, const VkApplicationInfo& appInfo,
                  DeviceSettings deviceSettings,
                  const SceneDependancies* sceneDependancies);
  virtual void cleanup();
  virtual void draw(const bool& framebufferResized, RenderSceneData& sceneData);

 private:
  void createSyncObjects();

  // Core Rendering Components
  RenderPass* renderPass;
  Descriptor* graphicsDescriptorSetLayout;
  Descriptor* graphicsMaterialDescriptorSetLayout;
  std::vector<GraphicsPipeline*> graphicsPipelines;
  std::vector<ComputePipeline*> computePipelines;
  // Render Data Managers
  ModelBufferManager* modelBufferManager;
  UI* ui;

  std::vector<VkSemaphore> imageAvailableSemaphores;
  std::vector<VkSemaphore> renderFinishedSemaphores;
  std::vector<VkFence> inFlightFences;
};