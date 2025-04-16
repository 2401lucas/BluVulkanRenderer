#ifndef FORWARDRENDERER_H
#define FORWARDRENDERER_H

#ifdef _DEBUG
constexpr bool USE_VALIDATION = true;
#else   // _DEBUG
constexpr bool USE_VALIDATION = false;
#endif  // _RELEASE

#include <EASTL/array.h>
#include <EASTL/hash_map.h>
#include <EASTL/queue.h>
#include <vk_mem_alloc.h>

#include <glm/mat4x4.hpp>

#include "../External/Window.h"
#include "Components/ModelData.h"
#include "ForwardRendererConsts.h"
#include "RenderData.h"
#include "Stages/AntiAliasingStage.h"
#include "Stages/BuildCommandBufferStage.h"
#include "Stages/DepthOnlyStage.h"
#include "Stages/FrustumCullStage.h"
#include "Stages/ImGuiStage.h"
#include "Stages/ImageCopyStage.h"
#include "Stages/OpaqueRenderStage.h"
#include "Vulkan/Buffer.h"
#include "Vulkan/DescriptorSet.h"
#include "Vulkan/Device.h"
#include "Vulkan/Image.h"
#include "Vulkan/Instance.h"
#include "Vulkan/Pipeline.h"
#include "Vulkan/Swapchain.h"

struct Vertex {
  glm::vec3 pos;
  glm::vec3 norm;
  glm::vec3 uv;
};

enum CullingMode {
  CULLING_MODE_NONE,
  CULLING_MODE_FRUSTUM_CULL,
  CULLING_MODE_OCCLUSION_CULL,
};

enum DrawMode {
  DRAW_MODE_SHADED,
  DRAW_MODE_UNLIT,
  DRAW_MODE_WIREFRAME,
};

enum AntiAliasingMode {
  ANTI_ALIAS_MODE_NONE,
  ANTI_ALIAS_MODE_FXAA,
};

enum RenderOutput {
  RENDER_OUTPUT_DRAW_STAGE,
  RENDER_OUTPUT_AA,
};

struct RenderSettings {
  CullingMode culling_mode = CULLING_MODE_FRUSTUM_CULL;
  DrawMode draw_mode = DRAW_MODE_SHADED;
  AntiAliasingMode aliasing = ANTI_ALIAS_MODE_NONE;
  RenderOutput output = RenderOutput::RENDER_OUTPUT_DRAW_STAGE;
};

// Contains all draw related data
struct ModelIndices {
  // Model Data
  int vert_offset;
  uint32_t ind_count;
  uint32_t ind_offset;
  // Texture Data
  uint32_t material_type;
  int base_tex_id;
  int normal_tex_id;
  int emission_tex_id;
  int metalness_tex_id;
  int diffuse_roughness_id;
  int ambient_occlusion_id;
};

struct ModelInfo {
  int model_ids;
  // Used for Culling
  glm::vec4 model_bounding_box;
};

struct DCGPushConst {
  BufferInfo input_model_data;
  BufferInfo input_models;
  BufferInfo output_command_data;
  uint32_t draw_count;
  uint32_t workgroup_size;
};

enum RendererState {
  OK = 0,
  ASPECT_RATIO_UPDATED = 1 << 0,
};

enum MaterialType {
  SINGLE_TEXTURE = 0,
  METALLIC_ROUGHNESS = 1 << 0,
};

class ForwardRenderer {
 public:
  ForwardRenderer(blu::core::Window* window);
  ~ForwardRenderer();

  eastl::vector<ModelInfo> LoadModel(eastl::string filepath);
  int LoadModel(blu::core::rendering::ModelData);

  int LoadImage(eastl::string filepath);
  void LoadTexture(blu::core::rendering::Material::TextureInfo&,
                   eastl::string folderpath);

  void GenerateResources();
  void UpdateFrameData(RenderData& render_data);
  void BuildFrameTimeline();
  bool PrepareFrame();
  bool PresentFrame(blu::core::Image* target_image, uint64_t wait_semaphore);

  RendererState Render(RenderData render_data);

  float GetAspectRatio() {
    return (float)swapchain_->GetWidth() / swapchain_->GetHeight();
  };

 private:
  uint64_t GetNextSemaphoreValue();
  void OnResize();

  void DoCull(uint32_t model_count);
  void DoDraw();
  void DoAA();
  bool DoPresent();

  void StartCommandBuffer(VkCommandBuffer);
  void EndCommandBuffer(VkCommandBuffer);

  void SubmitCommandBuffer(eastl::vector<VkCommandBuffer> cmd_bufs,
                           VkQueue& queue, VkSemaphore wait_semaphore,
                           uint64_t wait_value, VkPipelineStageFlags wait_flag,
                           VkSemaphore signal_semaphore, uint64_t signal_value,
                           VkFence fence);

  VkPipelineShaderStageCreateInfo LoadShader(eastl::string file_name,
                                             VkShaderStageFlagBits);

#ifdef _DEBUG
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
  } debug_util;
#endif

  RenderSettings settings_;

  blu::core::Window* window_;

  blu::core::Instance* instance_;
  blu::core::Device* device_;
  blu::core::Swapchain* swapchain_;
  VmaAllocator allocator_;

  // Render Data
  uint32_t frame_index_ = 0;
  uint32_t image_index_ = 0;
  uint64_t current_semaphore_value = 0;

  struct TimelineSemaphoreValues {
    // Complete Operations
    uint64_t cull_mode_complete = 0;
    uint64_t draw_mode_complete = 0;
    uint64_t anti_aliasing_mode_complete = 0;
    uint64_t present_complete = 0;

    // Individual Stages
    uint64_t build_command_buffer_stage_ = 0;
    uint64_t frustum_cull_stage_ = 0;
    uint64_t depth_only_stage_ = 0;
    uint64_t occlusion_cull_stage_ = 0;
    uint64_t opaque_render_stage_ = 0;
    uint64_t unlit_opaque_render_stage_ = 0;
    uint64_t wireframe_render_stage_ = 0;
    uint64_t image_copy_stage_ = 0;
    uint64_t anti_aliasing_stage_ = 0;
  } semaphore_values;

  // MODEL INFO:
  eastl::hash_map<eastl::string, uint32_t> loaded_texture_indices_;
  eastl::hash_map<eastl::string, uint32_t> loaded_model_indices_;
  // Raw Data information
  eastl::vector<blu::core::rendering::ModelData> loaded_models_;
  eastl::vector<ModelIndices> model_indices_;

  // Vulkan Render Data
  // Descriptor Resources
  VkDescriptorPool render_descriptor_pool_;

  blu::core::DescriptorSet* buffer_infos_descriptor_set_;
  blu::core::Buffer* buffer_infos_buffer_;
  eastl::vector<BufferInfo> buffer_infos_;
  blu::core::DescriptorSet* textures_descriptor_set_;
  eastl::vector<blu::core::Image*> textures;

  blu::core::Buffer* vertex_buffer_;
  uint32_t vertex_buffer_data_count_ = 0;
  blu::core::Buffer* normal_buffer_;
  uint32_t normal_buffer_data_count_ = 0;
  blu::core::Buffer* index_buffer_;
  uint32_t index_buffer_data_count_ = 0;
  blu::core::Buffer* uv_buffer_;
  uint32_t uv_buffer_data_count_ = 0;

  bool models_data_buffer_updated = false;
  blu::core::Buffer* models_data_buffer_;
  bool models_buffer_updated = false;
  blu::core::Buffer* models_buffer_;

  blu::core::Buffer* matrices_buffer_;

  // Vulkan Render Resources
  eastl::vector<VkShaderModule> shader_modules_;

  eastl::vector<VkCommandPool> transfer_command_pools;
  eastl::vector<VkCommandPool> graphics_command_pools_;
  eastl::vector<VkCommandPool> compute_command_pools_;

  eastl::vector<VkCommandBuffer> cmd_bufs_cull_;
  eastl::vector<VkCommandBuffer> cmd_bufs_draw_;
  eastl::vector<VkCommandBuffer> cmd_bufs_present_;

  eastl::vector<blu::core::Buffer*> buffers_draw_command_;
  eastl::vector<blu::core::Image*> images_render_assist_color;
  eastl::vector<blu::core::Image*> images_render_assist_depth;

  blu::core::rendering::BuildCommandBufferStage* build_command_buffer_stage_ =
      nullptr;
  blu::core::rendering::FrustumCullStage* frustum_cull_stage_ = nullptr;
  blu::core::rendering::DepthOnlyStage* depth_only_stage_ = nullptr;
  blu::core::rendering::ImageCopyStage* image_copy_stage_ = nullptr;
  blu::core::rendering::OpaqueRenderStage* opaque_render_stage_ = nullptr;
  blu::core::rendering::ImGuiStage* imgui_stage_ = nullptr;
  // blu::core::rendering::AntiAliasingStage* anti_aliasing_stage_ = nullptr;

  blu::core::rendering::Stage* hierarchial_z_stage_;
  blu::core::rendering::Stage* occlusion_cull_stage_;
  blu::core::rendering::Stage* post_process_stage_;
  blu::core::rendering::Stage* image_blit_stage_;

  VkSemaphore frame_semaphore;
  eastl::vector<VkSemaphore> present_semaphores;
  eastl::vector<VkSemaphore> image_available_semaphore;
  eastl::vector<VkFence> in_flight_fences_;
};

#endif