#ifndef SWAPCHAIN_H
#define SWAPCHAIN_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include "../../External/Window.h"
#include "Device.h"

namespace blu::core {
class Swapchain {
 public:
  Swapchain(blu::core::Instance* vk_instance, blu::core::Device* vk_device,
            blu::core::Window* window);
  ~Swapchain();

  void Create(bool vsync, bool fullscreen);

  VkSwapchainKHR GetSwapchain() const { return swapchain_; }
  VkFormat GetColorFormat() const { return color_format_; }
  uint32_t GetImageCount() const { return image_count_; }
  uint32_t GetWidth() const { return image_width_; }
  uint32_t GetHeight() const { return image_height_; }

 private:
  typedef struct SwapchainBuffer {
    VkImage image;
    VkImageView view;
  };

  blu::core::Instance* vk_instance_;
  blu::core::Device* vk_device_;
  blu::core::Window* window_;

  VkSurfaceKHR surface_;
  VkSwapchainKHR swapchain_ = VK_NULL_HANDLE;
  VkFormat color_format_;
  VkColorSpaceKHR color_space_;
  uint32_t image_count_;
  eastl::vector<VkImage> images_;
  eastl::vector<SwapchainBuffer> buffers_;
  uint32_t image_width_;
  uint32_t image_height_;
};
}  // namespace blu::core
#endif