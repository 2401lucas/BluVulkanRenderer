#ifndef BLUCORERENDERER_H
#define BLUCORERENDERER_H

#include <EASTL/array.h>
#include <EASTL/hash_map.h>
#include <EASTL/string.h>
#include <EASTL/vector.h>
#include <vk_mem_alloc.h>

#include <glm/glm.hpp>

#include "../External/Window.h"
#include "BluRendererConsts.h"
#include "Components/ModelData.h"
#include "GpuStructs.h"
#include "Vulkan/Device.h"
#include "Vulkan/Image.h"
#include "Vulkan/Instance.h"
#include "Vulkan/Stage.h"
#include "Vulkan/Swapchain.h"

class BluCoreRenderer {
 public:
  BluCoreRenderer(blu::core::Window* window);
  ~BluCoreRenderer();

  // TODO: break down Scene model info (IE: Hierarchy) and send info to engine
  struct LoadedModelInfo {
    int model_id;
    // int parent_model_id;
    glm::vec4 model_bounding_box;
  };

  eastl::vector<LoadedModelInfo> LoadModel(eastl::string filepath);
  int LoadModel(blu::core::rendering::ModelData);

  int LoadImage(eastl::string filepath);
  void LoadTexture(blu::core::rendering::Material::TextureInfo&,
                   eastl::string folderpath);

  void Build();

 protected:
  struct CommandPool {
    VkCommandPool pool;
    uint32_t last_used = 0;
    eastl::array<VkCommandBuffer, 64> allocated_command_buffers;
  };

  virtual void Resize() = 0;

  uint64_t GetNextSemaphoreValue();

  void StartCommandBuffer(VkCommandBuffer, const char* name, glm::vec4 rgb);
  void EndCommandBuffer(VkCommandBuffer);

  void SubmitCommandBuffer(eastl::vector<VkCommandBuffer> cmd_bufs,
                           VkQueue& queue,
                           eastl::vector<VkSemaphore> wait_semaphores,
                           eastl::vector<uint64_t> wait_values,
                           eastl::vector<VkPipelineStageFlags> wait_flags,
                           eastl::vector<VkSemaphore> signal_semaphores,
                           eastl::vector<uint64_t> signal_values,
                           VkFence fence);

  VkPipelineShaderStageCreateInfo LoadShader(eastl::string file_name,
                                             VkShaderStageFlagBits);
  blu::core::Buffer* CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                                  VkMemoryPropertyFlags required_flags,
                                  VmaAllocationCreateFlags flags = 0);
  void RegisterStage(blu::core::rendering::Stage*);

  blu::core::Image* CreateRenderTargetImage(
      VkFormat format, uint32_t width, uint32_t height, uint32_t mip_levels,
      VkSampleCountFlagBits samples, VkImageTiling tiling,
      VkImageUsageFlags usage, VkMemoryPropertyFlags required_flags,
      VmaAllocationCreateFlags flags = 0);
  blu::core::Buffer* CreateRenderTargetBuffer(
      VkDeviceSize size, VkBufferUsageFlags usage,
      VkMemoryPropertyFlags required_flags, VmaAllocationCreateFlags flags = 0);
  VkSemaphore& CreateSemaphore(VkSemaphoreCreateInfo&);
  VkFence& CreateFence(VkFenceCreateInfo&);
  CommandPool& CreateCommandPool(VkCommandPoolCreateInfo&);
  VkCommandBuffer& AllocateCommandBuffers(CommandPool,
                                          VkCommandBufferAllocateInfo&);
  VkDescriptorPool& CreateDescriptorPool(VkDescriptorPoolCreateInfo&);
  VkDescriptorSetLayout& CreateDescriptorSetLayout(
      VkDescriptorSetLayoutCreateInfo&);

 protected:
#ifdef DEBUG_LABELS
  inline void BeginLabel(VkCommandBuffer cmd, const char* name, glm::vec3 rgb) {
    VkDebugUtilsLabelEXT label = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
        .pLabelName = name,
        .color = {rgb.x, rgb.y, rgb.z, 1},
    };
    vkCmdBeginDebugUtilsLabelEXT(cmd, &label);
  }
  inline void EndLabel(VkCommandBuffer cmd) { vkCmdEndDebugUtilsLabelEXT(cmd); }
  inline void InsertLabel(VkCommandBuffer cmd, const char* name,
                          glm::vec3 rgb) {
    VkDebugUtilsLabelEXT label = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
        .pLabelName = name,
        .color = {rgb.x, rgb.y, rgb.z, 1},
    };
    vkCmdInsertDebugUtilsLabelEXT(cmd, &label);
  }
  inline void NameObject(VkDevice device, uint64_t handle, VkObjectType type,
                         const char* name) {
    VkDebugUtilsObjectNameInfoEXT nameInfo = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
        .objectType = type,
        .objectHandle = handle,
        .pObjectName = name,
    };
    vkSetDebugUtilsObjectNameEXT(device, &nameInfo);
  }

  struct DebugUtils {
    PFN_vkCmdBeginDebugUtilsLabelEXT vkCmdBeginDebugUtilsLabelEXT;
    PFN_vkCmdEndDebugUtilsLabelEXT vkCmdEndDebugUtilsLabelEXT;
    PFN_vkCmdInsertDebugUtilsLabelEXT vkCmdInsertDebugUtilsLabelEXT;
    PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectNameEXT;
  } debug_util_;
#endif

  struct Pass {
    eastl::vector<VkCommandBuffer> cmd_bufs_;
    eastl::vector<VkSemaphore> semaphores_;
    eastl::vector<VkFence> fences_;
  };

  // External Managers
  blu::core::Window* window_;

  // Vulkan Wrappers
  blu::core::Instance* instance_;
  blu::core::Device* device_;
  blu::core::Swapchain* swapchain_;
  VmaAllocator allocator_;

  uint32_t frame_index_ = 0;
  uint32_t image_index_ = 0;
  uint32_t current_semaphore_value_ = 0;

  uint32_t vertex_buffer_data_count_ = 0;
  uint32_t normal_buffer_data_count_ = 0;
  uint32_t index_buffer_data_count_ = 0;
  uint32_t uv_buffer_data_count_ = 0;
  blu::core::Buffer* vertex_buffer_;
  blu::core::Buffer* normal_buffer_;
  blu::core::Buffer* index_buffer_;
  blu::core::Buffer* uv_buffer_;

  VkDescriptorSetLayout bda_buffer_descriptor_set_layout_;
  VkDescriptorSet bda_buffer_descriptor_set_;
  eastl::vector<BufferInfo> bda_buffer_infos_;
  blu::core::Buffer* bda_buffer_;

  VkDescriptorSetLayout textures_descriptor_set_layout_;
  VkDescriptorSet textures_descriptor_set_;

  // Raw Model (Mesh+Material) Data
  eastl::vector<GPUModelIndices> model_indices_;

 private:
  // Cached Model Info
  eastl::hash_map<eastl::string, uint32_t> loaded_texture_indices_;
  eastl::hash_map<eastl::string, uint32_t> loaded_model_indices_;

  // GLOBAL Vulkan Resources
  eastl::hash_map<eastl::string, VkShaderModule> shader_modules_;
  eastl::hash_map<eastl::string, VkSampler> image_samplers_;

  eastl::vector<VkSemaphore> semaphores_;
  eastl::vector<VkFence> fences_;
  eastl::vector<CommandPool> command_pools_;
  eastl::vector<VkDescriptorPool> descriptor_pools_;
  eastl::vector<VkDescriptorSetLayout> descriptor_set_layouts;

  eastl::vector<blu::core::Image*> model_textures_;

  eastl::vector<blu::core::Buffer*> generic_buffers_;

  eastl::vector<blu::core::Buffer*> render_buffers_;
  eastl::vector<blu::core::Image*> render_images_;

  eastl::vector<blu::core::rendering::Stage*> loaded_stages;

  // Per Thread Vulkan Resources
  struct ThreadResources {
    VkCommandPool pool;
  };
};
#endif