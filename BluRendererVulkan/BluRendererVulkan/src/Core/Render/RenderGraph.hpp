#pragma once

#include <vulkan/vulkan.h>

#include <functional>
#include <string>
#include <vector>

#include "ResourceManagement/VulkanResources/VulkanDevice.h"
#include "ResourceManagement/VulkanResources/VulkanSwapchain.h"
#include "ResourceManagement/VulkanResources/VulkanTexture.h"

// TODO: Remove vulkan specific resources
namespace core_internal::rendering {
class RenderGraph;
class RenderGraphPass;

enum class AttachmentSizeRelative {
  SwapchainRelative,
  AbsoluteValue,
};

enum RenderGraphQueueFlagBits {
  RENDER_GRAPH_GRAPHICS_QUEUE = 1 << 0,
  RENDER_GRAPH_COMPUTE_QUEUE = 1 << 1,
  RENDER_GRAPH_TRANSFER_QUEUE = 1 << 2,
};
using RenderGraphQueueFlags = uint32_t;

enum RenderGraphMemoryTypeFlagBits {
  MEMORY_GPU_ONLY = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
  // Size Limit is small, bigger data should be uploaded with a staging buffer
  MEMORY_CPU_TO_GPU = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
  MEMORY_GPU_TO_CPU = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
                      VK_MEMORY_PROPERTY_HOST_CACHED_BIT,
};
using RenderGraphMemoryTypeFlags = uint32_t;

struct AttachmentInfo {
  RenderGraphMemoryTypeFlags memoryFlags;
  AttachmentSizeRelative sizeRelative =
      AttachmentSizeRelative::SwapchainRelative;
  float sizeX = 1.0f;
  float sizeY = 1.0f;
  VkFormat format = VK_FORMAT_UNDEFINED;
  VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
  uint32_t mipLevels = 1;
  uint32_t arrayLayers = 1;
  uint32_t depth = 1;
  VkImageUsageFlags usage;
  bool requireSampler = false;
  bool requireImageView = false;
  bool requireMappedData = false;
  bool persistant = false;  // Static data can still be updated upon request
};

struct BufferInfo {
  VkDeviceSize size = 0;
  VkBufferUsageFlags usage = 0;
  RenderGraphMemoryTypeFlags memoryFlags;

  bool persistant = false;  // Static data can still be updated upon request
  // If data is Persistant, this forces one buffer to be shared for each frame
  // in flight
  bool forceSingleInstance = false;
};

class RenderResource {
 public:
  // Used for Generating Dependency Layers
  std::vector<uint32_t> usedInPasses;
  // Resource Lifespan only cares about first/last time used
  unsigned long resourceLifespan;

  bool persistant = false;
  uint32_t resourceIndex = -1;

  RenderResource(uint32_t index) { usedInPasses.push_back(index); }
  void registerPass(uint32_t index) { usedInPasses.push_back(index); }
};

class RenderTextureResource : public RenderResource {
 public:
  AttachmentInfo renderTextureInfo;

  RenderTextureResource();
};

class RenderBufferResource : public RenderResource {
 public:
  BufferInfo renderBufferInfo;

  RenderBufferResource();
};

class RenderGraphPass {
 private:
  RenderGraph *graph;
  RenderGraphQueueFlags queue = RENDER_GRAPH_GRAPHICS_QUEUE;
  uint32_t index;

  std::vector<RenderTextureResource *> inputTextureAttachments;
  std::vector<RenderBufferResource *> inputStorageAttachments;
  std::vector<RenderTextureResource *> outputColorAttachments;
  std::vector<RenderBufferResource *> outputStorageAttachments;
  RenderTextureResource *depthStencilInput;
  RenderTextureResource *depthStencilOutput;

  std::function<bool(VkClearDepthStencilValue *)> getDepthClearColor_cb;
  std::function<bool(uint32_t, VkClearColorValue *)> getColorClearColor_cb;
  std::function<void()> getCommandBuffer_cb;

 public:
  // Acyclic Dependency Layer
  uint32_t dependencyLayer = -1;

  void addAttachmentInput(const std::string &name);
  RenderTextureResource *addColorOutput(const std::string &name,
                                        AttachmentInfo info);

  void setDepthStencilInput(const std::string &name);
  void setDepthStencilOutput(const std::string &name,
                             const AttachmentInfo &info);
  RenderBufferResource *addStorageInput(const std::string &name);
  RenderBufferResource *addStorageOutput(const std::string &name,
                                         const BufferInfo &info);

  void set_GetCommandBuffer(std::function<void()> callback);

  void setQueue(const RenderGraphQueueFlags &queue);
  RenderGraphQueueFlags &getQueue();
  void checkQueue(const uint32_t &passID,
                  const RenderGraphQueueFlags &passQueue);

  std::vector<RenderTextureResource *> &getOutputAttachments();
};

// TODO: INSERT BLOG LINK RELATING TO RENDER GRAPH IMPLEMENTATION
// Since this is controlling all frame resources, it should be able to keep
// track of exact VRAM size and other relevant info. Should have it's own
// toggleable Debug UI
class RenderGraph {
 private:
  core_internal::rendering::vulkan::VulkanDevice *device;
  core_internal::rendering::vulkan::VulkanSwapchain *swapchain;

  // Registered Resources
  std::vector<RenderGraphPass *> renderGraphPasses;
  std::unordered_map<std::string, RenderTextureResource *> textureBlackboard;
  std::unordered_map<std::string, RenderBufferResource *> bufferBlackboard;

  // Unsure about format
  RenderTextureResource *finalOutput = nullptr;

  // GPU Resources
  std::vector<vulkan::Image *> renderImages;
  std::vector<vulkan::Buffer *> renderBuffers;
  std::vector<vulkan::Image *> internalRenderImages;
  std::vector<vulkan::Buffer *> internalRenderBuffers;

  // Rendering Resources
  struct GeneratedRenderGraph {};
  std::vector<GeneratedRenderGraph *> genRenderGraph;

  void validateData();

  void generateDependencyChain();
  void dependencySearch(const uint32_t &nodeIndex, const uint32_t &depth);
  void generateResources();

  uint32_t getImageSize(AttachmentSizeRelative sizeRelative,
                        uint32_t swapchainSize, float size);

  void generateMemoryBarriers();
  void generateDescriptorSets();

 public:
  RenderGraph();
  ~RenderGraph();

  void prepareFrame();
  VkResult submitFrame();
  void onResized();

  // Should outside updated resources be managed by RenderGraph, or by outside
  // classes
  // VkPipelineStageFlags is only required for pipeline, I am unsure how
  // pipelines will be handled but they probably will be externally managed,
  // making this obselete?
  // Also when creating render passes with Dynamic Rendering, very few should
  // be used and it also requires explicite image transfers
  RenderGraphPass *addPass(const std::string &name,
                           const VkPipelineStageFlags flag);

  RenderTextureResource *createTexture(const std::string &name,
                                       const AttachmentInfo &info);
  RenderBufferResource *createBuffer(const std::string &name,
                                     const BufferInfo &info);

  RenderTextureResource *getTexture(const std::string &name);
  RenderBufferResource *getBuffer(const std::string &name);

  RenderBufferResource *setFinalOutput(const std::string &name);

  void printRenderGraph();

  void bake();

  // Access to modify baked resource memory such as model position Buffers
};
}  // namespace core_internal::rendering