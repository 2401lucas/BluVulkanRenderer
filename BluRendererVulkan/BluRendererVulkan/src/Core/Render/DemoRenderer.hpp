#pragma once

#define DEBUG_ALL 1
// #define DEBUG_RENDERGRAPH
// #define DEBUG_RENDERER
// #define DEBUG_ENGINE

#include "BaseRenderer.h"
#include "Components/Camera.hpp"
#include "Components/Light.hpp"

class DemoRenderer : public BaseRenderer {
 public:
  DemoRenderer() : BaseRenderer() {}

  void getEnabledFeatures() override {  // This is a memory leak lmao
    VkPhysicalDeviceDynamicRenderingFeaturesKHR* dynamicRenderingpNext{};
    dynamicRenderingpNext->sType =
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR;
    dynamicRenderingpNext->dynamicRendering = VK_TRUE;
    pNextChain = dynamicRenderingpNext;

    VkPhysicalDeviceDescriptorIndexingFeaturesEXT* descriptorIndexingpNext{};
    dynamicRenderingpNext->pNext = descriptorIndexingpNext;
    descriptorIndexingpNext->sType =
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT;
    descriptorIndexingpNext->runtimeDescriptorArray = VK_TRUE;

    VkPhysicalDeviceBufferDeviceAddressFeaturesKHR* bufferDeviceAddresspNext{};
    descriptorIndexingpNext->pNext = bufferDeviceAddresspNext;
    bufferDeviceAddresspNext->sType =
        VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO_KHR;
    bufferDeviceAddresspNext->bufferDeviceAddress = VK_TRUE;

    VkPhysicalDeviceSynchronization2FeaturesKHR* deviceSuncpNext{};
    bufferDeviceAddresspNext->pNext = deviceSuncpNext;
    deviceSuncpNext->sType =
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES_KHR;
    deviceSuncpNext->synchronization2 = VK_TRUE;
  }

  void getEnabledExtensions() override {
    enabledInstanceExtensions.push_back(
        VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

    enabledDeviceExtensions.push_back(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);
    enabledDeviceExtensions.push_back(
        VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);
    enabledDeviceExtensions.push_back(
        VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);

    // TODO: CREATE INFO
    enabledDeviceExtensions.push_back(VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME);
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
    auto mainPass = renderGraph->addPass(
        "Mainpass",
        core_internal::rendering::RenderGraph::DrawType::CPU_RECORDED);
    mainPass->set_GetCommandBuffer([](VkCommandBuffer buf) {
      // For Each Model
      // vkCmdBindIndexBuffer(buf, IndexBuffer(), 0, VK_INDEX_TYPE_UINT32);
      // vkCmdBindVertexBuffers(buf, 0, 1, VertexBuffer(), 0);
      // Draw();
    });
    mainPass->addColorOutput("MainColorOutput",
                             {.format = vulkanSwapchain->colorFormat});
    renderGraph->setFinalOutput("MainColorOutput");
  }

  void mainpassRender(VkCommandBuffer buf) {}

  void render() override {}

  void windowResized() override {}
};