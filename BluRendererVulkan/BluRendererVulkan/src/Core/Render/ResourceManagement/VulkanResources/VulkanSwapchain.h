#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include <vector>

#include "VulkanDevice.h"

namespace core_internal::rendering {
typedef struct _SwapChainBuffers {
  VkImage image;
  VkImageView view;
} SwapChainBuffer;

class VulkanSwapchain {
 private:
  core_internal::rendering::VulkanDevice* vulkanDevice;
  VkSurfaceKHR surface;

 public:
  VkFormat colorFormat;
  VkColorSpaceKHR colorSpace;
  VkSwapchainKHR swapChain = VK_NULL_HANDLE;
  uint32_t imageCount;
  std::vector<VkImage> images;
  std::vector<SwapChainBuffer> buffers;
  uint32_t graphicsQueueNodeIndex = UINT32_MAX;
  uint32_t computeQueueNodeIndex = UINT32_MAX;
  uint32_t imageWidth;
  uint32_t imageHeight;

  VulkanSwapchain(core_internal::rendering::VulkanDevice*, GLFWwindow*);
  ~VulkanSwapchain();

  void create(int* width, int* height, bool vsync = false,
              bool fullscreen = false);
  VkResult acquireNextImage(VkSemaphore presentCompleteSemaphore,
                            uint32_t* imageIndex);
  VkResult queuePresent(VkQueue queue, uint32_t imageIndex,
                        VkSemaphore* waitSemaphore = VK_NULL_HANDLE);
};
}  // namespace core_internal::rendering