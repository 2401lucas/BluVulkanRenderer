#ifndef FORWARDRENDERER_H
#define FORWARDRENDERER_H

#ifdef _DEBUG
constexpr bool USE_VALIDATION = true;
#else   // _RELEASE
constexpr bool USE_VALIDATION = false;
#endif  // _DEBUG

#include <EASTL/array.h>
#include <EASTL/hash_map.h>
#include <EASTL/queue.h>
#include <vk_mem_alloc.h>

#include <glm/mat4x4.hpp>

#include "../Components/Model.h"
#include "../External/Window.h"
#include "RenderData.h"
#include "Vulkan/Buffer.h"
#include "Vulkan/DescriptorSet.h"
#include "Vulkan/Device.h"
#include "Vulkan/Image.h"
#include "Vulkan/Instance.h"
#include "Vulkan/Pipeline.h"
#include "Vulkan/Swapchain.h"

constexpr uint32_t MAX_MODELS = 10;
constexpr uint32_t MAX_VERTICES = 1000;
constexpr uint32_t MAX_INDICES = 1000;
constexpr uint32_t MAX_TEXTURES = 10;
constexpr VkFormat DEPTH_FORMAT = VK_FORMAT_D32_SFLOAT;
constexpr VkFormat COLOR_FORMAT = VK_FORMAT_R8G8B8A8_SRGB;
constexpr VkDeviceSize MAX_BUFFERS_STORAGE = 32;
constexpr VkDeviceSize DRAW_COMMAND_BUFFER_SIZE =
    sizeof(VkDrawIndexedIndirectCommand);

struct BufferInfo {
  VkDeviceAddress address;
  VkDeviceSize offset;
  VkDeviceSize size;
};

struct Vertex {
  glm::vec3 pos;
  glm::vec3 norm;
  glm::vec3 uv;
};

// Contains all draw related data
struct ModelIndices {
  // Model Data
  uint32_t vert_offset;
  uint32_t ind_count;
  uint32_t ind_offset;
  // Texture Data
  uint32_t material_type;
  int main_tex_id;
  int secondary_tex_id;
  int tertiary_tex_id;
};

struct DCGPushConst {
  BufferInfo input_model_data;
  BufferInfo input_models;
  BufferInfo output_command_data;
  uint32_t draw_count;
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

  int LoadModel(eastl::string filepath);
  int LoadModel(blu::core::components::Model);

  int LoadImage(eastl::string filepath);

  void Prepare();
  RendererState Render(RenderData render_data);

  float GetAspectRatio() {
    return (float)swapchain_->GetWidth() / swapchain_->GetHeight();
  };

 private:
  void OnResize();

  VkPipelineShaderStageCreateInfo LoadShader(eastl::string file_name,
                                             VkShaderStageFlagBits);

  blu::core::Window* window_;

  blu::core::Instance* instance_;
  blu::core::Device* device_;
  blu::core::Swapchain* swapchain_;
  VmaAllocator allocator_;

  // Render Data
  uint32_t frame_index_ = 0;
  uint32_t image_index_ = 0;
  eastl::vector<BufferInfo> buffer_infos_;

  // MODEL INFO:
  eastl::hash_map<eastl::string, uint32_t> loaded_texture_indices_;
  eastl::hash_map<eastl::string, uint32_t> loaded_model_indices_;
  // Raw Data information
  eastl::vector<blu::core::components::Model> loaded_models_;
  eastl::vector<ModelIndices> model_indices_;

  // Vulkan Render Data
  // Descriptor Resources
  VkDescriptorPool render_descriptor_pool_;

  blu::core::DescriptorSet* buffer_infos_descriptor_set_;
  blu::core::Buffer* buffer_infos_buffer_;

  blu::core::Buffer* vertex_buffer_;
  uint32_t vertex_buffer_offset_ = 0;
  blu::core::Buffer* normal_buffer_;
  uint32_t normal_buffer_offset_ = 0;
  blu::core::Buffer* index_buffer_;
  uint32_t index_buffer_offset_ = 0;
  blu::core::Buffer* uv_buffer_;
  uint32_t uv_buffer_offset_ = 0;

  eastl::vector<blu::core::Buffer*> dcg_input_model_data_;
  eastl::vector<blu::core::Buffer*> dcg_input_models_;
  eastl::vector<blu::core::Buffer*> dcg_output_buffers_;

  blu::core::Buffer* matrices_buffer_;

  blu::core::DescriptorSet* textures_descriptor_set_;
  eastl::vector<blu::core::Image*> textures;

  // Vulkan Render Resources
  eastl::vector<VkShaderModule> shader_modules_;

  eastl::vector<VkCommandPool> transfer_command_pools;
  eastl::vector<VkCommandPool> graphics_command_pools_;
  eastl::vector<VkCommandPool> compute_command_pools_;

  eastl::vector<VkCommandBuffer> draw_command_buffers;
  eastl::vector<VkCommandBuffer> dcg_buffers;

  blu::core::Image* depth_stencil_image_;

  blu::core::rendering::Pipeline* dcg_pipeline_;
  blu::core::rendering::Pipeline* triangle_pipeline_;
  blu::core::rendering::Pipeline* cube_pipeline_;

  eastl::vector<VkSemaphore> image_available_semaphores_;
  eastl::vector<VkSemaphore> render_finished_semaphores_;
  eastl::vector<VkSemaphore> dcg_semaphores_;
  eastl::vector<VkFence> in_flight_fences_;
};

#endif