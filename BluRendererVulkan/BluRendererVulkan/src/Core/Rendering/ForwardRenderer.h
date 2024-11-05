#ifndef BLU_RENDERING_FORWARDRENDERER_H
#define BLU_RENDERING_FORWARDRENDERER_H

#ifdef _DEBUG
constexpr bool USE_VALIDATION = true;
#else   // _RELEASE
constexpr bool USE_VALIDATION = false;
#endif  // _DEBUG

#include "../Engine/Engine.h"
#include "../External/Window.h"
#include "Vulkan/DescriptorSet.h"
#include "Vulkan/Device.h"
#include "Vulkan/Instance.h"
#include "Vulkan/Swapchain.h"

constexpr int MAX_BUFFERS_STORAGE = 1;
constexpr int VERTEX_BUFFER_SIZE = sizeof(uint32_t) * 3 * 100;
constexpr int INDEX_BUFFER_SIZE = sizeof(uint32_t) * 100;

struct BufferInfo {
  VkDeviceAddress address;
  VkDeviceSize offset;
  VkDeviceSize size;
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
  blu::core::Buffer* CreateVertexBuffer();

  blu::core::Window* window_;

  blu::core::Instance* instance_;
  blu::core::Device* device_;
  blu::core::Swapchain* swapchain_;
  VmaAllocator allocator_;

  // Render Data
  uint32_t frame_index_;
  eastl::vector<glm::mat4> matrices_;
  eastl::vector<BufferInfo> buffer_infos_;
  eastl::vector<ModelIndices> model_indices_;

  // Vulkan Render Data Resources
  VkDescriptorPool descriptor_pool_;

  blu::core::Buffer* buffer_infos_buffer_;
  blu::core::DescriptorSet* buffer_infos_descriptor_set_;

  blu::core::Buffer* matrices_buffer_;
  blu::core::Buffer* vertex_buffer_;
  blu::core::Buffer* index_buffer_;

  blu::core::Buffer* model_indices_buffer_;

  // Vulkan Render Resources

};

#endif