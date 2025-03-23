#ifndef VULKANTOOLS_H
#define VULKANTOOLS_H

#include <vulkan/vulkan.h>

#include <iostream>

// Custom define for better code readability
#define VK_FLAGS_NONE 0
// Default fence timeout in nanoseconds
#define DEFAULT_FENCE_TIMEOUT 100000000000

// Macro to check and display Vulkan return results
#define VK_CHECK_RESULT(f)                                                  \
  {                                                                         \
    VkResult res = (f);                                                     \
    if (res != VK_SUCCESS) {                                                \
      std::cerr << "Fatal : VkResult is \"" << errorString(res) << "\" in " \
                << __FILE__ << " at line " << __LINE__ << "\n";             \
      EASTL_ASSERT(res == VK_SUCCESS);                           \
    }                                                                       \
  }
#endif

std::string errorString(VkResult errorCode);