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
    // TODO: CALCULATE EXTERNAL BUFFER SIZE REQUIREMENTS
    //  Vert/Ind buf should be mostly static, and could be updated with a
    //  staging buffer probably
    renderGraph->createBuffer(
        "meshInfo", {.size = 1,
                     .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                     .memoryFlags = core_internal::rendering::
                         RenderGraphMemoryTypeFlagBits::MEMORY_GPU_ONLY,
                     .isExternal = true,
                     .requireMappedData = false});

    // This should contain less data that is optimally packed and
    // only updated once per frame
    renderGraph->createBuffer(
        "modelInfo", {.size = 1,
                      .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                      .memoryFlags = core_internal::rendering::
                          RenderGraphMemoryTypeFlagBits::MEMORY_CPU_TO_GPU,
                      .isExternal = true,
                      .requireMappedData = true});
    auto generateRenderCommands = renderGraph->addPass(
        "generateRenderCommands",
        core_internal::rendering::RenderGraph::DrawType::DRAW_TYPE_COMPUTE);

    auto antiAliasingPass = renderGraph->addPass(
        "AntiAliasingPass", core_internal::rendering::RenderGraph::DrawType::
                                DRAW_TYPE_FULLSCREEN_TRIANGLE);
    antiAliasingPass->addColorOutput(
        "AAOut", {
                     .sizeRelative = core_internal::rendering::
                         AttachmentSizeRelative::SwapchainRelative,
                     .sizeX = 0.5,
                     .sizeY = 0.5,
                     .format = VK_FORMAT_R16_SFLOAT,
                 });

    auto mainPass = renderGraph->addPass(
        "Mainpass", core_internal::rendering::RenderGraph::DrawType::
                        DRAW_TYPE_CAMERA_OCCLUDED_OPAQUE);
    mainPass->addAttachmentInput("AAOut");
    mainPass->addStorageInput("meshInfo");
    mainPass->addStorageInput("modelInfo");
    mainPass->addColorOutput("MainColorOutput",
                             {.format = vulkanSwapchain->colorFormat});
    renderGraph->setFinalOutput("MainColorOutput");
  }

  void render() override {}

  void windowResized() override {}
};