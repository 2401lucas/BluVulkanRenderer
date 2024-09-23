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
#include "ResourceManagement/VulkanResources/VulkanTexture.h"

// TODO: Make Agnostic Render API
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

enum ShaderStagesFlagBits {
  SHADER_STAGE_VERTEX = 1 << 0,
  SHADER_STAGE_FRAGMENT = 1 << 1,
  SHADER_STAGE_COMPUTE = 1 << 2,

  SHADER_STAGE_VERT_FRAG = SHADER_STAGE_VERTEX + SHADER_STAGE_FRAGMENT,
};
using ShaderStagesFlag = uint32_t;

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
  VkShaderStageFlags shaderStageFlag = VK_SHADER_STAGE_FRAGMENT_BIT;
  bool requireSampler = false;
  bool requireImageView = false;
  bool requireMappedData = false;
  bool isExternal = false;  // Static data can still be updated upon request
};

struct BufferInfo {
  VkDeviceSize size = 0;
  VkBufferUsageFlags usage = 0;
  RenderGraphMemoryTypeFlags memoryFlags;
  VkShaderStageFlags shaderStageFlag = VK_SHADER_STAGE_FRAGMENT_BIT;
  VkDescriptorType type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

  bool isExternal = false;
  bool requireMappedData = false;
};

class RenderResource {
 public:
  // Used for Generating Dependency Layers
  std::vector<uint32_t> usedInPasses;
  // Resource Lifespan only cares about first/last time used
  unsigned long resourceLifespan = 0;
  uint32_t resourceIndex = -1;

  void registerPass(uint32_t index) { usedInPasses.push_back(index); }
};

class RenderTextureResource : public RenderResource {
 public:
  AttachmentInfo renderTextureInfo;

  RenderTextureResource() = delete;
  RenderTextureResource(AttachmentInfo renderTextureInfo) {
    this->renderTextureInfo = renderTextureInfo;
  }
};

class RenderBufferResource : public RenderResource {
 public:
  BufferInfo renderBufferInfo;

  RenderBufferResource() = delete;
  RenderBufferResource(BufferInfo renderBufferInfo) {
    this->renderBufferInfo = renderBufferInfo;
  }
};

class RenderGraphPass {
 private:
  RenderGraph *graph;
  RenderGraphQueueFlags queue = RENDER_GRAPH_GRAPHICS_QUEUE;
  uint32_t index;
  std::string name;

  RenderGraph::DrawType drawType;

  // VK Resources
  VkPipeline pipeline;
  VkPipelineLayout pipelineLayout;
  VkDescriptorSet descriptorSet;
  VkDescriptorSetLayout descriptorSetLayout;
  glm::vec3 computeGroups = glm::vec3(256, 1, 1);
  glm::vec2 size;

  std::vector<RenderTextureResource *> inputTextureAttachments;
  std::vector<RenderBufferResource *> inputStorageAttachments;
  std::vector<RenderTextureResource *> outputColorAttachments;
  std::vector<RenderBufferResource *> outputStorageAttachments;
  RenderTextureResource *depthStencilInput;
  RenderTextureResource *depthStencilOutput;

  std::function<bool(VkClearDepthStencilValue *)> getDepthClearColor_cb;
  std::function<bool(uint32_t, VkClearColorValue *)> getColorClearColor_cb;
  std::function<void(VkCommandBuffer)> recordCommandBuffer_cb;

  std::vector<std::pair<ShaderStagesFlag, std::string>> shaders;

 public:
  // Acyclic Dependency Layer
  uint32_t dependencyLayer = -1;

  RenderGraphPass(RenderGraph *, uint32_t index, const std::string &name);
  void registerShader(std::pair<ShaderStagesFlag, std::string> shader);

  void setComputeGroup(glm::vec3 computeGroup);
  void setSize(glm::vec2 size);
  void addAttachmentInput(const std::string &name);
  RenderTextureResource *addColorOutput(const std::string &name,
                                        AttachmentInfo info);

  void setDepthStencilInput(const std::string &name);
  void setDepthStencilOutput(const std::string &name,
                             const AttachmentInfo &info);
  RenderBufferResource *addStorageInput(const std::string &name);
  RenderBufferResource *addStorageOutput(const std::string &name,
                                         const BufferInfo &info);

  void set_GetCommandBuffer(std::function<void(VkCommandBuffer)> callback);

  void setQueue(const RenderGraphQueueFlags &queue);
  RenderGraphQueueFlags &getQueue();
  void checkQueue(const uint32_t &passID,
                  const RenderGraphQueueFlags &passQueue);

  std::vector<RenderTextureResource *> &getOutputAttachments();
  std::vector<RenderTextureResource *> &getInputAttachments();
  std::vector<RenderBufferResource *> &getInputStorage();
  std::vector<std::pair<ShaderStagesFlag, std::string>> &getShaders();
  std::string getName();

  void draw(VkCommandBuffer buf);

  void createDescriptorSetLayout(VkDevice, VkDescriptorSetLayoutCreateInfo *);
};

// TODO: INSERT BLOG LINK RELATING TO RENDER GRAPH IMPLEMENTATION
// Since this is controlling all frame resources, it should be able to keep
// track of exact VRAM size and other relevant info. Should have it's own
// toggleable Debug UI
// Assumptions:
// Each RGPass is only wrote to once? Is this a required assumption
class RenderGraph {
 private:
  // A Model is defined as an object to be rendered containing both Mesh &
  // Texture Info
  struct RegisteredModel {
    // VkDrawIndexedIndirectCommand renderData;
    // ShaderMaterial shaderModelInfo;
    int meshIndex;
    int materialIndex;
  };

  struct RenderingSettings {
    bool useMSAA;
  };
  core_internal::rendering::vulkan::VulkanDevice *device;
  core_internal::rendering::vulkan::VulkanSwapchain *swapchain;

  // Registered Resources
  std::vector<RenderGraphPass *> renderPasses;
  std::unordered_map<std::string, RenderTextureResource *> textureBlackboard;
  std::unordered_map<std::string, RenderBufferResource *> bufferBlackboard;

  // Unsure about format
  RenderTextureResource *finalOutput = nullptr;

  // GPU Resources
  std::vector<vulkan::Image *> renderImages;
  std::vector<vulkan::Buffer *> renderBuffers;
  std::vector<vulkan::Image *> internalRenderImages;
  std::vector<vulkan::Buffer *> internalRenderBuffers;

  std::vector<RegisteredModel> registeredModels;
  std::vector<uint32_t> models;
  std::queue<uint32_t> emptyModelSlots;

  // Cached GPU resources
  std::unordered_map<std::string, uint32_t> modelBlackboard;
  std::unordered_map<std::string, uint32_t> materialBlackboard;

  std::vector<vulkan::Image *> shaderImages;  // Texture Images

  vulkan::Buffer vertexBuffer;  // Could be broken into more buffers
  vulkan::Buffer indexBuffer;

  // Rendering Resources
  VkSemaphore timelineSemaphore;
  VkSemaphore presentSemaphore;

  std::vector<vulkan::Buffer *> drawBuffers;

  // Indexed by GL_InstanceIndex
  struct GPUIndices {
    int posIndex;
    int colorIndex;
    int physicalDescriptorIndex;
    int normalIndex;
    int aoIndex;
    int emmisiveIndex;
  };

  struct GPUMeshData {
    glm::mat4 mvp;
  };

  void validateData();

  void generateDependencyChain();
  void dependencySearch(const uint32_t &nodeIndex, const uint32_t &depth);
  void generateResources();

  uint32_t getImageSize(AttachmentSizeRelative sizeRelative,
                        uint32_t swapchainSize, float size);

  void generateDescriptorSets();
  void generateSemaphores();

 public:
  enum class DrawType {
    CameraOccludedOpaque,
    CameraOccludedTranslucent,
    FullscreenTriangle,
    Compute,          // Not actually triangle, but used for perf
                      // testing
    CustomOcclusion,  // Requires Bounding box info
    CPU_RECORDED,     // Requires callback to draw commands to be supplied
  };

  // Bindless descriptor set used when rendering models
  VkDescriptorSet modelDescriptorSet;

  RenderGraph();
  ~RenderGraph();

  // All (Aliased) Resources need an MemoryBarrier to specify previous memory as
  // undefined
  // Timeline Semaphores to sync each Dependency Layer + Binary Semaphore to
  // sync q submit
  // All Scene Info should be stored in 2 buffers
  // Buffer 1: Vertex/Index Buffer (Mesh Info)
  // Buffer 2: Material, Transforms & Maybe Camera info?
  // Buffer 3: Output buffer of draw commands?
  // How to handle UI? (UI managed by RG, allowing for RG specific UI(Debugging)
  // + Scene unique UI)
  void prepareFrame();
  VkResult submitFrame();
  void onResized();

  // Used to clean generated model assets, used on scene change.
  void clearModels();

  // Registers a Model and it's required rendering info onto the GPU
  // TODO: Upload Queue to GPU to improve frametimes when creating models
  // If Mesh or Texture data does not already exist on the GPU, there is a
  // multi-frame delay expected. Mesh & Texture data can take up to X frames,
  // and if there is a Queue, it could take even longer
  // desiredUploadSize:
  //   When calculating what to upload each frame, this is the ideal size
  // maxFramesForUpload:
  //   If the data to upload is too large, the data size is divided by
  //   maxFramesForUpload to calculate data upload
  // (dataSize / desiredUploadSize > maxFramesForUpload)
  // maxObjecsInQueue:
  //   If the upload queue gets too long, the cap for upload per frame is
  //   raised until the queue size is reduced. This is to keep object creations
  //   delays reasonable
  uint32_t registerModel(core::engine::components::Model *);

  // Should outside updated resources be managed by RenderGraph, or by outside
  // classes
  // VkPipelineStageFlags is only required for pipeline, I am unsure how
  // pipelines will be handled but they probably will be externally managed,
  // making this obselete?
  // Also when creating render passes with Dynamic Rendering, very few should
  // be used and it also requires explicite image transfers
  RenderGraphPass *addPass(const std::string &name, const DrawType &);

  RenderTextureResource *createTexture(const std::string &name,
                                       const AttachmentInfo &info);
  RenderBufferResource *createBuffer(const std::string &name,
                                     const BufferInfo &info);

  RenderTextureResource *getTexture(const std::string &name);
  vulkan::Image *getTexture(const uint32_t &index);
  RenderBufferResource *getBuffer(const std::string &name);

  RenderBufferResource *setFinalOutput(const std::string &name);

  VkBuffer *getVertexBuffer();
  VkBuffer getIndexBuffer();

  void bake();

  vulkan::Buffer getDrawBuffer(DrawType);
  // Access to modify baked resource memory such as model position Buffers
};
}  // namespace core_internal::rendering