#pragma once

#define DEBUG_ALL
// #define DEBUG_RENDERGRAPH
// #define DEBUG_RENDERER
// #define DEBUG_ENGINE

#include "BaseRenderer.h"
#include "Components/Components.h"

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
    enabledDeviceExtensions.push_back(VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME);
  }

  void buildEngine() override {
    // engine = new core_internal::engine::BaseEngine();
  }

  void prepare() override {
    prepareExternalBuffers();
    buildRenderGraph();
    renderGraph->bake();
  }

  void prepareExternalBuffers() {
    /*core_internal::rendering::Buffer* buf = {};

    VkBufferCreateInfo bufCI{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = 0,
        .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
    };

    vulkanDevice->createBuffer();
    vulkanDevice->copyMemoryToAlloc();

    renderGraph->registerExternalData("thing", buf);*/
  }
  // External Data
  // Images are added to a container that are sent to/read from the GPU
  // Image Index is saved at time of creation : (default tex index 0 could be
  // set by default, or not rendered until model is uploaded? INVESTIGATE)
  // Model Struct holds
  // CPU INFO:
  //   Filepaths(Bypass read into ram, send straigt to gpu memory?)
  // GPU INFO:
  // -Generic Model:
  //    Resource Indices
  //    Vertex/Index Buffers Indices?
  // -Rendered Model
  //    World Tranform
  //    Model Index for resources
  void buildRenderGraph() {
    renderGraph->registerExternalData("namr",
                                      new core_internal::rendering::Buffer());

    auto mainP = renderGraph->addGraphicsPass(
        {{"cubeSpin.vert.spv", VK_PIPELINE_STAGE_VERTEX_SHADER_BIT},
         {"cubeSpin.frag.spv", VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT}});

    mainP->addOutput("mainOut", VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);

    mainP->addInput("ubnot", 0);
  }

  void mainpassRender(VkCommandBuffer buf) {}

  void render() override {}

  void windowResized() override {}
};