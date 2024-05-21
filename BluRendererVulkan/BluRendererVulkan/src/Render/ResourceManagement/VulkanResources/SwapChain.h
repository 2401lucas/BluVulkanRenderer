#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include <vector>

typedef struct _SwapChainBuffers {
  VkImage image;
  VkImageView view;
} SwapChainBuffer;

class SwapChain {
 private:
  VkInstance instance;
  VkDevice device;
  VkPhysicalDevice physicalDevice;
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

  void initSurface(GLFWwindow*);
  void connect(VkInstance, VkPhysicalDevice, VkDevice);
  void create(uint32_t* width, uint32_t* height, bool vsync = false,
              bool fullscreen = false);
  VkResult acquireNextImage(VkSemaphore presentCompleteSemaphore,
                            uint32_t* imageIndex);
  VkResult queuePresent(VkQueue queue, uint32_t imageIndex,
                        VkSemaphore* waitSemaphore = VK_NULL_HANDLE);
  void cleanup();
};