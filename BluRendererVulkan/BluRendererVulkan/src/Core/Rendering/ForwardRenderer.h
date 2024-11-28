#ifndef FORWARDRENDERER_H
#define FORWARDRENDERER_H

#ifdef _DEBUG
constexpr bool USE_VALIDATION = true;
#else   // _RELEASE
constexpr bool USE_VALIDATION = false;
#endif  // _DEBUG

#include <EASTL/array.h>
#include <assimp/scene.h>  // Output data structure
#include <vk_mem_alloc.h>

#include "../Engine/Engine.h"
#include "../External/Window.h"
#include "Vulkan/Buffer.h"
#include "Vulkan/DescriptorSet.h"
#include "Vulkan/Device.h"
#include "Vulkan/Image.h"
#include "Vulkan/Instance.h"
#include "Vulkan/Pipeline.h"
#include "Vulkan/Swapchain.h"

constexpr VkFormat DEPTH_FORMAT = VK_FORMAT_D32_SFLOAT;
constexpr VkDeviceSize MAX_BUFFERS_STORAGE = 3;
constexpr VkDeviceSize VERTEX_BUFFER_SIZE = sizeof(aiVector3D) * 1000;
constexpr VkDeviceSize INDEX_BUFFER_SIZE = sizeof(uint32_t) * 1000;
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

struct ModelIndices {
  int mesh_id;
  int transform_id;
};

class ForwardRenderer {
 public:
  ForwardRenderer(blu::core::Window* window);
  ~ForwardRenderer();

  void Prepare();
  void Render(blu::core::Engine::RenderData render_data);

 private:
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
  eastl::vector<glm::mat4> matrices_;
  eastl::vector<BufferInfo> buffer_infos_;
  eastl::vector<ModelIndices> model_indices_;

  uint32_t vert_count_;
  uint32_t ind_count_;

  // Vulkan Render Data Resources
  VkCommandPool transfer_command_pool;
  VkCommandPool* graphics_command_pools_;
  eastl::vector<VkCommandBuffer> draw_command_buffers_;
  VkDescriptorPool descriptor_pool_;

  blu::core::Image* depth_stencil_image_;

  blu::core::Buffer* buffer_infos_buffer_;
  blu::core::DescriptorSet* buffer_infos_descriptor_set_;

  blu::core::Buffer* matrices_buffer_;
  blu::core::Buffer* vertex_buffer_;
  blu::core::Buffer* index_buffer_;

  // Vulkan Render Resources
  eastl::vector<VkShaderModule> shader_modules_;
  blu::core::rendering::Pipeline* triangle_pipeline_;
  eastl::vector<VkSemaphore> image_available_semaphores_;
  eastl::vector<VkSemaphore> render_finished_semaphores_;
  eastl::vector<VkFence> in_flight_fences_;
};

#endif