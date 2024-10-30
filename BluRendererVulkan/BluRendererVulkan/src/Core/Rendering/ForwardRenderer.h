#ifndef BLU_RENDERING_FORWARDRENDERER_H
#define BLU_RENDERING_FORWARDRENDERER_H

#ifdef _DEBUG
constexpr bool USE_VALIDATION = true;
#else // _RELEASE
constexpr bool USE_VALIDATION = false;
#endif  // _DEBUG


#include "Vulkan/Device.h"
#include "Vulkan/Instance.h"
#include "Vulkan/Swapchain.h"

class ForwardRenderer {
 public:
  ForwardRenderer();
  ~ForwardRenderer();

  void Render();

 private:
   vk::core::Instance* instance_;
   vk::core::Device* device_;
   vk::core::Swapchain* swapchain_;


  // Window Info
   int width_;
   int height_;
};

#endif