#ifndef BLU_RENDERING_FORWARDRENDERER_H
#define BLU_RENDERING_FORWARDRENDERER_H

#ifdef _DEBUG
constexpr bool USE_VALIDATION = true;
#else   // _RELEASE
constexpr bool USE_VALIDATION = false;
#endif  // _DEBUG

#include "../External/Window.h"
#include "Vulkan/Device.h"
#include "Vulkan/Instance.h"
#include "Vulkan/Swapchain.h"

class ForwardRenderer {
 public:
  ForwardRenderer();
  ~ForwardRenderer();

  void Prepare();
  void Render();

 private:
  blu::core::Instance* instance_;
  blu::core::Device* device_;
  blu::core::Swapchain* swapchain_;
  blu::core::Window* window_;

  uint32_t frame_index_;
};

#endif