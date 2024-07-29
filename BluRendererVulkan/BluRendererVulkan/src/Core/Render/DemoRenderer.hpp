#pragma once

#define DEBUG_ALL
#define DEBUG_RENDERGRAPH
#define DEBUG_RENDERER
#define DEBUG_ENGINE

#include "BaseRenderer.h"
#include "Components/Camera.hpp"
#include "Components/Light.hpp"

class DemoRenderer : public BaseRenderer {
 public:
  DemoRenderer() : BaseRenderer() {}

  void getEnabledFeatures() override {
    VkPhysicalDeviceDynamicRenderingFeaturesKHR* dynamicRenderingpNext{};
    dynamicRenderingpNext->dynamicRendering = VK_TRUE;
    pNextChain = dynamicRenderingpNext;

    VkPhysicalDeviceDescriptorIndexingFeaturesEXT* descriptorIndexingpNext{};
    dynamicRenderingpNext->pNext = descriptorIndexingpNext;
    descriptorIndexingpNext->runtimeDescriptorArray = VK_TRUE;
  }

  void getEnabledExtensions() override {
    enabledInstanceExtensions.push_back(
        VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

    enabledDeviceExtensions.push_back(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);
    enabledDeviceExtensions.push_back(
        VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);
  }

  void buildEngine() override {
    // engine = new core_internal::engine::BaseEngine();
  }

  void prepare() override {
    prepareExternalBuffers();
    buildRenderGraph();
  }

  void prepareExternalBuffers() {}

  void buildRenderGraph() {
    auto mainPass =
        renderGraph->addPass("Mainpass", VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT);
    mainPass->addColorOutput("MainColorOutput",
                             {.format = vulkanSwapchain->colorFormat});
    renderGraph->setFinalOutput("MainColorOutput");
  }

  void render() override {}

  void windowResized() override {}
};