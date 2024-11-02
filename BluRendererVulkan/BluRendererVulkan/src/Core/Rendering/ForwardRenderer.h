#ifndef BLU_RENDERING_FORWARDRENDERER_H
#define BLU_RENDERING_FORWARDRENDERER_H

#ifdef _DEBUG
constexpr bool USE_VALIDATION = true;
#else   // _RELEASE
constexpr bool USE_VALIDATION = false;
#endif  // _DEBUG

#include "../Engine/Engine.h"
#include "../External/Window.h"
#include "Vulkan/Device.h"
#include "Vulkan/Instance.h"
#include "Vulkan/Swapchain.h"

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
  blu::core::Window* window_;

  blu::core::Instance* instance_;
  blu::core::Device* device_;
  blu::core::Swapchain* swapchain_;

  uint32_t frame_index_;

  eastl::vector<glm::mat4> matrices_;

  eastl::vector<ModelIndices> model_indices;
};

#endif