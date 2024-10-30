#ifndef VULKANSWAPCHAIN_H
#define VULKANSWAPCHAIN_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include "Device.h"

namespace vk::core {
class Swapchain {
 public:
  Swapchain(vk::core::Instance* vk_instance, vk::core::Device* vk_device,
            GLFWwindow* window);
  ~Swapchain();

  void Create(int* width, int* height, bool vsync, bool fullscreen);

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

  vk::core::Instance* vk_instance_;
  vk::core::Device* vk_device_;

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
}  // namespace vk::core
#endif