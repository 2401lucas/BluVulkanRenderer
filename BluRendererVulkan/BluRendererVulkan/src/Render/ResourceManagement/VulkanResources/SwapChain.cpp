#include "SwapChain.h"

#include "../VulkanResources/VulkanTools.h"

void SwapChain::initSurface(GLFWwindow* window) {
  VK_CHECK_RESULT(glfwCreateWindowSurface(instance, window, nullptr, &surface));
  uint32_t queueCount;
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueCount, NULL);
  assert(queueCount >= 1);

  std::vector<VkQueueFamilyProperties> queueProps(queueCount);
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueCount,
                                           queueProps.data());

  // Iterate over each queue to learn whether it supports presenting:
  // Find a queue with present support
  // Will be used to present the swap chain images to the windowing system
  std::vector<VkBool32> supportsPresent(queueCount);
  for (uint32_t i = 0; i < queueCount; i++) {
    vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface,
                                         &supportsPresent[i]);
  }

  // Search for a graphics and a present queue in the array of queue
  // families, try to find one that supports both
  uint32_t graphicsQueueNodeIndex = UINT32_MAX;
  uint32_t computeQueueNodeIndex = UINT32_MAX;
  uint32_t presentQueueNodeIndex = UINT32_MAX;
  for (uint32_t i = 0; i < queueCount; i++) {
    if ((queueProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0) {
      if (graphicsQueueNodeIndex == UINT32_MAX) {
        graphicsQueueNodeIndex = i;
      }

      if (supportsPresent[i] == VK_TRUE) {
        graphicsQueueNodeIndex = i;
        presentQueueNodeIndex = i;
        break;
      }
    }
  }
  if (presentQueueNodeIndex == UINT32_MAX) {
    // If there's no queue that supports both present and graphics
    // try to find a separate present queue
    for (uint32_t i = 0; i < queueCount; ++i) {
      if (supportsPresent[i] == VK_TRUE) {
        presentQueueNodeIndex = i;
        break;
      }
    }
  }

  for (uint32_t i = 0; i < queueCount; i++) {
    if ((queueProps[i].queueFlags & VK_QUEUE_COMPUTE_BIT) != 0) {
      if (computeQueueNodeIndex == UINT32_MAX) {
        computeQueueNodeIndex = i;
        break;
      }
    }
  }

  // Exit if either a graphics or a presenting queue hasn't been found
  if (graphicsQueueNodeIndex == UINT32_MAX ||
      presentQueueNodeIndex == UINT32_MAX ||
      computeQueueNodeIndex == UINT32_MAX) {
    vks::tools::exitFatal("Could not find a graphics and/or presenting queue!",
                          -1);
  }

  if (graphicsQueueNodeIndex != presentQueueNodeIndex) {
    vks::tools::exitFatal(
        "Separate graphics and presenting queues are not supported yet!", -1);
  }

  this->graphicsQueueNodeIndex = graphicsQueueNodeIndex;
  this->computeQueueNodeIndex = computeQueueNodeIndex;

  // Get list of supported surface formats
  uint32_t formatCount;
  VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface,
                                                       &formatCount, NULL));
  assert(formatCount > 0);

  std::vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
  VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(
      physicalDevice, surface, &formatCount, surfaceFormats.data()));

  // We want to get a format that best suits our needs, so we try to get one
  // from a set of preferred formats Initialize the format to the first one
  // returned by the implementation in case we can't find one of the preffered
  // formats
  VkSurfaceFormatKHR selectedFormat = surfaceFormats[0];
  std::vector<VkFormat> preferredImageFormats = {
      VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM,
      VK_FORMAT_A8B8G8R8_UNORM_PACK32};

  for (auto& availableFormat : surfaceFormats) {
    if (std::find(preferredImageFormats.begin(), preferredImageFormats.end(),
                  availableFormat.format) != preferredImageFormats.end()) {
      selectedFormat = availableFormat;
      break;
    }
  }

  colorFormat = selectedFormat.format;
  colorSpace = selectedFormat.colorSpace;
}

void SwapChain::connect(VkInstance instance, VkPhysicalDevice physicalDevice,
                        VkDevice device) {
  this->instance = instance;
  this->physicalDevice = physicalDevice;
  this->device = device;
}

void SwapChain::create(uint32_t* width, uint32_t* height, bool vsync,
                       bool fullscreen) {
  VkSwapchainKHR oldSwapchain = swapChain;

  VkSurfaceCapabilitiesKHR surfCaps;
  VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
      physicalDevice, surface, &surfCaps));

  uint32_t presentModeCount;
  VK_CHECK_RESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(
      physicalDevice, surface, &presentModeCount, NULL));

  std::vector<VkPresentModeKHR> presentModes(presentModeCount);
  VK_CHECK_RESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(
      physicalDevice, surface, &presentModeCount, presentModes.data()));

  VkExtent2D swapchainExtent = {};
  // If width (and height) equals the special value 0xFFFFFFFF, the size of the
  // surface will be set by the swapchain
  if (surfCaps.currentExtent.width == (uint32_t)-1) {
    // If the surface size is undefined, the size is set to
    // the size of the images requested.
    swapchainExtent.width = *width;
    swapchainExtent.height = *height;
  } else {
    // If the surface size is defined, the swap chain size must match
    swapchainExtent = surfCaps.currentExtent;
    *width = surfCaps.currentExtent.width;
    *height = surfCaps.currentExtent.height;
  }

  VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;

  if (!vsync) {
    for (size_t i = 0; i < presentModeCount; i++) {
      if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
        swapchainPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
        break;
      }
      if (presentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR) {
        swapchainPresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
      }
    }
  }

  uint32_t desiredNumberOfSwapchainImages = surfCaps.minImageCount + 1;

  if ((surfCaps.maxImageCount > 0) &&
      (desiredNumberOfSwapchainImages > surfCaps.maxImageCount)) {
    desiredNumberOfSwapchainImages = surfCaps.maxImageCount;
  }

  VkSurfaceTransformFlagsKHR preTransform;
  if (surfCaps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
    preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
  } else {
    preTransform = surfCaps.currentTransform;
  }
  VkCompositeAlphaFlagBitsKHR compositeAlpha =
      VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  // Simply select the first composite alpha format available
  std::vector<VkCompositeAlphaFlagBitsKHR> compositeAlphaFlags = {
      VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
      VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
      VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
      VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
  };
  for (auto& compositeAlphaFlag : compositeAlphaFlags) {
    if (surfCaps.supportedCompositeAlpha & compositeAlphaFlag) {
      compositeAlpha = compositeAlphaFlag;
      break;
    };
  }

  VkSwapchainCreateInfoKHR swapchainCI = {};
  swapchainCI.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  swapchainCI.surface = surface;
  swapchainCI.minImageCount = desiredNumberOfSwapchainImages;
  swapchainCI.imageFormat = colorFormat;
  swapchainCI.imageColorSpace = colorSpace;
  swapchainCI.imageExtent = {swapchainExtent.width, swapchainExtent.height};
  swapchainCI.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  swapchainCI.preTransform = (VkSurfaceTransformFlagBitsKHR)preTransform;
  swapchainCI.imageArrayLayers = 1;
  swapchainCI.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  swapchainCI.queueFamilyIndexCount = 0;
  swapchainCI.presentMode = swapchainPresentMode;
  // Setting oldSwapChain to the saved handle of the previous swapchain aids
  // in resource reuse and makes sure that we can still present already
  // acquired images
  swapchainCI.oldSwapchain = oldSwapchain;
  // Setting clipped to VK_TRUE allows the implementation to discard rendering
  // outside of the surface area
  swapchainCI.clipped = VK_TRUE;
  swapchainCI.compositeAlpha = compositeAlpha;

  // Enable transfer source on swap chain images if supported
  if (surfCaps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) {
    swapchainCI.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
  }

  // Enable transfer destination on swap chain images if supported
  if (surfCaps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT) {
    swapchainCI.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  }

  VK_CHECK_RESULT(
      vkCreateSwapchainKHR(device, &swapchainCI, nullptr, &swapChain));

  // If an existing swap chain is re-created, destroy the old swap chain
  // This also cleans up all the presentable images
  if (oldSwapchain != VK_NULL_HANDLE) {
    for (uint32_t i = 0; i < imageCount; i++) {
      vkDestroyImageView(device, buffers[i].view, nullptr);
    }
    vkDestroySwapchainKHR(device, oldSwapchain, nullptr);
  }
  VK_CHECK_RESULT(
      vkGetSwapchainImagesKHR(device, swapChain, &imageCount, NULL));

  // Get the swap chain images
  images.resize(imageCount);
  VK_CHECK_RESULT(
      vkGetSwapchainImagesKHR(device, swapChain, &imageCount, images.data()));

  // Get the swap chain buffers containing the image and imageview
  buffers.resize(imageCount);
  for (uint32_t i = 0; i < imageCount; i++) {
    VkImageViewCreateInfo colorAttachmentView = {};
    colorAttachmentView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    colorAttachmentView.pNext = NULL;
    colorAttachmentView.format = colorFormat;
    colorAttachmentView.components = {
        VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B,
        VK_COMPONENT_SWIZZLE_A};
    colorAttachmentView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    colorAttachmentView.subresourceRange.baseMipLevel = 0;
    colorAttachmentView.subresourceRange.levelCount = 1;
    colorAttachmentView.subresourceRange.baseArrayLayer = 0;
    colorAttachmentView.subresourceRange.layerCount = 1;
    colorAttachmentView.viewType = VK_IMAGE_VIEW_TYPE_2D;
    colorAttachmentView.flags = 0;

    buffers[i].image = images[i];

    colorAttachmentView.image = buffers[i].image;

    VK_CHECK_RESULT(vkCreateImageView(device, &colorAttachmentView, nullptr,
                                      &buffers[i].view));
  }
}

VkResult SwapChain::acquireNextImage(VkSemaphore presentCompleteSemaphore,
                                     uint32_t* imageIndex) {
  return vkAcquireNextImageKHR(device, swapChain, UINT64_MAX,
                               presentCompleteSemaphore, (VkFence) nullptr,
                               imageIndex);
}

VkResult SwapChain::queuePresent(VkQueue queue, uint32_t imageIndex,
                                 VkSemaphore* waitSemaphore) {
  VkPresentInfoKHR presentInfo = {};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.pNext = NULL;
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = &swapChain;
  presentInfo.pImageIndices = &imageIndex;
  // Check if a wait semaphore has been specified to wait for before presenting
  // the image
  if (waitSemaphore != VK_NULL_HANDLE) {
    presentInfo.pWaitSemaphores = waitSemaphore;
    presentInfo.waitSemaphoreCount = 1;
  }
  return vkQueuePresentKHR(queue, &presentInfo);
}

void SwapChain::cleanup() {
  if (swapChain != VK_NULL_HANDLE) {
    for (uint32_t i = 0; i < imageCount; i++) {
      vkDestroyImageView(device, buffers[i].view, nullptr);
    }
  }
  if (surface != VK_NULL_HANDLE) {
    vkDestroySwapchainKHR(device, swapChain, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);
  }
  surface = VK_NULL_HANDLE;
  swapChain = VK_NULL_HANDLE;
}