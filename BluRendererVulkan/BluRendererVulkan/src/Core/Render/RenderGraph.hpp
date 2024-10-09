#pragma once

#include <vulkan/vulkan.h>

#include <functional>
#include <glm/ext/vector_float4.hpp>
#include <glm/mat4x4.hpp>
#include <queue>
#include <string>
#include <vector>

#include "Components/Model.hpp"
#include "ResourceManagement/VulkanResources/VulkanDevice.h"
#include "ResourceManagement/VulkanResources/VulkanSwapchain.h"

namespace core_internal::rendering::rendergraph {
class RenderGraphPass {
 public:
  enum class RenderGraphPassType { Graphics, Compute };

  struct OutputBufferInfo {
    std::string name;
    VkBufferUsageFlagBits usage;
    VkDeviceSize size;
  };

  struct OutputImageInfo {
    std::string name;
    VkImageUsageFlagBits usage;
    glm::vec2 size;
  };

 private:
  RenderGraphPassType passType;
  glm::ivec3 size;
  std::vector<std::string> shaders;

  std::vector<std::string> inputs;
  std::vector<OutputBufferInfo> outputBuffers;
  std::vector<OutputImageInfo> outputImages;

 public:
  RenderGraphPass(RenderGraphPassType, glm::ivec3 size,
                  std::vector<std::string> shaders);
  ~RenderGraphPass();

  void validateData();
  std::vector<std::string>& getInputs();
  std::vector<OutputImageInfo>& getImageOutputs();
  std::vector<OutputBufferInfo>& getBufferOutputs();

  void addInput(std::string resourceName);
  void addOutput(std::string name, VkBufferUsageFlagBits, VkDeviceSize size);
  void addOutput(std::string name, VkImageUsageFlagBits,
                 glm::vec2 size = {1, 1});
};

class RenderGraph {
 private:
  core_internal::rendering::VulkanDevice* vulkanDevice;
  // RenderGraph Info
  glm::ivec2 targetSize;

  struct RenderResource {
    std::vector<std::string> usedByResources;
    unsigned long memoryReservation = 0;  // Based on Dependency Layers
    VkDeviceSize size = 0;                // Based on Dependency Layers

    RenderResource(std::string name, unsigned long firstUse,
                   unsigned long lastUse, VkDeviceSize size)
        : size(size) {
      usedByResources.push_back(name);
      for (unsigned long i = firstUse; i < lastUse; i++) {
        memoryReservation |= (1 << i);
      }
    }
  };

  struct BuildInfo {
    std::vector<RenderGraphPass*> rgPasses;
    std::vector<uint32_t> rgDependencyLayer;
    std::vector<RenderResource> bufferReservations;
    std::vector<RenderResource> imageReservations;
  } buildInfo;

  struct BufferInfo {
    uint32_t rpOutputIndex;
    core_internal::rendering::Buffer* buf;
    RenderGraphPass::OutputBufferInfo bufInfo;
    uint32_t resourceLifespan;
    std::vector<uint32_t> usedIn;
  };

  struct ImageInfo {
    uint32_t rpOutputIndex;
    core_internal::rendering::Image* img;
    RenderGraphPass::OutputImageInfo imgInfo;
    uint32_t resourceLifespan;
    std::vector<uint32_t> usedIn;
  };

  struct BakedInfo {
    VkDeviceSize bakedVRAMBufferSizeAbsolute = 0;  // Total Size
    VkDeviceSize bakedVRAMImageSizeAbsolute = 0;   // Total Size
    VkDeviceSize bakedVRAMBufferSizeActual = 0;    // Optimized Size (Aliasing)
    VkDeviceSize bakedVRAMImageSizeActual = 0;     // Optimized Size (Aliasing)
    std::unordered_map<std::string, BufferInfo> bufferBlackboard;
    std::unordered_map<std::string, ImageInfo> imageBlackboard;
    std::string finalOutput;
  } bakedInfo;

 public:
  RenderGraph(core_internal::rendering::VulkanDevice*, glm::ivec2 targetSize);
  ~RenderGraph();

  void toggleDebugUI();

  void setTargetSize(glm::ivec2 targetSize);
  // setFinalOutput allows for already existing outputs to be set as the final
  // output without rebuilding the RenderGraph, useful for debugging
  void setFinalOutput(std::string finalOutput);

  void bake();

  void registerExternalData(std::string name,
                            core_internal::rendering::Buffer*);
  void registerExternalData(std::string name, core_internal::rendering::Image*);

  RenderGraphPass* addGraphicsPass(std::vector<std::string> shaders);
  RenderGraphPass* addComputePass(std::string computeShader,
                                  glm::ivec3 dispatchGroup);

 private:
  void getRenderpassData();
  void validateData();
  void dependencySearch(const uint32_t& passIndex, const uint32_t& depth);
  void generateDependencyChain();
  void generateResources();
  void generateBufferResourceReservations();
  void generateImageResourceReservations();
  void generateDescriptorSets();
  void generatePipelines();
  void generateSemaphores();
};
}  // namespace core_internal::rendering::rendergraph