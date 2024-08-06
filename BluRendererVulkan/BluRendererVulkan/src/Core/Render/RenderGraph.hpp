#pragma once

#include <vulkan/vulkan.h>

#include <functional>
#include <string>
#include <vector>

#include "ResourceManagement/VulkanResources/VulkanDevice.h"
#include "ResourceManagement/VulkanResources/VulkanSwapchain.h"

// TODO: Remove vulkan specific resources
namespace core_internal::rendering {
class RenderGraph;
class RenderGraphPass;

enum class AttachmentSizeRelative {
  Swapchain,
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
  RENDER_GRAPH_DEPTH_STENCIL_FRAME_MANAGED = 1 << 0,
  RENDER_GRAPH_COLOR_PERSISTANT = 1 << 1,
  RENDER_GRAPH_COLOR_FRAME_MANAGED = 1 << 2,
  RENDER_GRAPH_UNIFORM_BUFFER_FRAME_MANAGED = 1 << 3,
  RENDER_GRAPH_UNIFORM_BUFFER_PERSISTANT = 1 << 4,
  RENDER_GRAPH_STORAGE_BUFFER_FRAME_MANAGED = 1 << 5,
  RENDER_GRAPH_STORAGE_BUFFER_PERSISTANT = 1 << 6,
};
using RenderGraphMemoryTypeFlags = uint32_t;

struct AttachmentInfo {
  AttachmentSizeRelative sizeRelative = AttachmentSizeRelative::Swapchain;
  float sizeX = 1.0f;
  float sizeY = 1.0f;
  VkFormat format = VK_FORMAT_UNDEFINED;
  uint32_t samples = 1;
  uint32_t levels = 1;
  uint32_t layers = 1;
  bool persistant = false;  // Static data can still be updated upon request

  bool operator==(AttachmentInfo &info) {
    return sizeRelative == info.sizeRelative && sizeX == info.sizeX &&
           sizeY == info.sizeY && format == info.format &&
           samples == info.samples && levels == info.levels &&
           layers == info.layers;
  }
};

struct BufferInfo {
  VkDeviceSize size = 0;
  VkBufferUsageFlags usage = 0;
  bool persistant = false;  // Static data can still be updated upon request

  bool operator==(BufferInfo &info) {
    return size == info.size && usage == info.usage;
  }
};

class RenderResource {
 public:
  // Used for Generating Dependency Layers
  std::vector<uint32_t> usedInPasses;
  // Used for Resource Lifespan
  uint32_t maxDependencyRange = -1;
  uint32_t minDependencyRange = 0xffffffff;

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
                                        const AttachmentInfo &info);

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

  struct ResourceReservation {
    std::vector<bool> dependencyReservation;

    ResourceReservation() = default;

    ResourceReservation(uint32_t dependencyMin, uint32_t dependencyMax) {
      dependencyReservation.insert(dependencyReservation.end(), false,
                                   dependencyMax);

      for (uint32_t i = dependencyMin; i < dependencyMax; i++) {
        dependencyReservation[i] = true;
      }
    }
  };

  struct TextureResourceReservation : public ResourceReservation {
    AttachmentInfo texInfo;

    TextureResourceReservation() = default;

    TextureResourceReservation(AttachmentInfo texInfo, uint32_t dependencyMin,
                               uint32_t dependencyMax)
        : ResourceReservation(dependencyMin, dependencyMax), texInfo(texInfo) {}
  };

  struct BufferResourceReservation : public ResourceReservation {
    BufferInfo bufInfo;

    BufferResourceReservation() = default;

    BufferResourceReservation(BufferInfo bufInfo, uint32_t dependencyMin,
                              uint32_t dependencyMax)
        : ResourceReservation(dependencyMin, dependencyMax), bufInfo(bufInfo) {}
  };

  // Baked Resources
  struct GeneratedRenderGraph {};
  struct AllocatedTexture {};
  struct AllocatedBuffer {};

   std::vector<GeneratedRenderGraph *> genRenderGraph;
   std::vector<AllocatedBuffer *> bakedBuffers;
   std::vector<AllocatedTexture *> bakedTextures;

  void validateData();

  void generateDependencyChain();
  void dependencySearch(const uint32_t &nodeIndex, const uint32_t &depth);

  void generateResourceBuckets();

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

#ifdef DEBUG_RENDERGRAPH
  void printRenderGraph();
#endif  // DEBUG_RENDERGRAPH

  void bake();

  // Access to modify baked resource memory such as model position Buffers
};
}  // namespace core_internal::rendering