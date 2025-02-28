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

constexpr VkFormat DEPTH_FORMAT = VK_FORMAT_D32_SFLOAT;
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
  // eastl::array<float, 2> uv;
};

// Contains all draw related data
struct ModelIndices {
  uint32_t pipeline_index;
  uint32_t vert_count;
  uint32_t ind_count;
  uint32_t mesh_vert_buf_index;
  uint32_t mesh_norm_buf_index;
  uint32_t mesh_ind_buf_index;
  // int texture_id;
};

class ForwardRenderer {
 public:
  ForwardRenderer(blu::core::Window* window);
  ~ForwardRenderer();

  uint32_t LoadModel(eastl::string filepath);
  uint32_t LoadModel(blu::core::components::Model);

  void Prepare();
  void Render(RenderData render_data);

  void OnResize();

 private:
  VkPipelineShaderStageCreateInfo LoadShader(eastl::string file_name,
                                             VkShaderStageFlagBits);

  blu::core::Window* window_;

  blu::core::Instance* instance_;
  blu::core::Device* device_;
  blu::core::Swapchain* swapchain_;
  VmaAllocator allocator_;

  // Render Data
  // What is the difference?
  uint32_t frame_index_ = 0;
  uint32_t image_index_ = 0;
  eastl::vector<glm::mat4> matrices_;
  eastl::vector<BufferInfo> buffer_infos_;

  eastl::queue<eastl::string> model_loading_queue_;
  eastl::hash_map<eastl::string, uint32_t> loaded_model_indices_;
  // Raw Data information
  eastl::vector<blu::core::components::Model> loaded_models_;

  eastl::vector<ModelIndices> model_indices_;

  // Vulkan Render Data Resources
  VkDescriptorPool descriptor_pool_;

  VkCommandPool transfer_command_pool;
  VkCommandPool* graphics_command_pools_;

  VkCommandBuffer* draw_command_buffers;

  blu::core::Image* depth_stencil_image_;

  blu::core::Buffer* buffer_infos_buffer_;
  blu::core::DescriptorSet* buffer_infos_descriptor_set_;

  blu::core::Buffer* matrices_buffer_;
  eastl::vector<blu::core::Buffer*> vertex_buffers_;
  eastl::vector<blu::core::Buffer*> normal_buffers_;
  eastl::vector<blu::core::Buffer*> index_buffers_;

  // Vulkan Render Resources
  eastl::vector<VkShaderModule> shader_modules_;

  blu::core::rendering::Pipeline* triangle_pipeline_;
  blu::core::rendering::Pipeline* cube_pipeline_;

  eastl::vector<VkSemaphore> image_available_semaphores_;
  eastl::vector<VkSemaphore> render_finished_semaphores_;
  eastl::vector<VkFence> in_flight_fences_;
};

#endif