#pragma once
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "vulkan/vulkan.h"
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#include <windows.h>
#endif
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

namespace vks {
namespace debug {
// Default debug callback
VKAPI_ATTR VkBool32 VKAPI_CALL debugUtilsMessageCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);
// Load debug function pointers and set debug callback
void setupDebugging(VkInstance instance);
// Clear debug callback
void freeDebugCallback(VkInstance instance);
// Used to populate a VkDebugUtilsMessengerCreateInfoEXT with our example
// messenger function and desired flags
void setupDebugingMessengerCreateInfo(
    VkDebugUtilsMessengerCreateInfoEXT& debugUtilsMessengerCI);

}  // namespace debug
   // Wrapper for the VK_EXT_debug_utils extension
// These can be used to name Vulkan objects for debugging tools like RenderDoc
namespace debugutils {
void setup(VkInstance instance);
void cmdBeginLabel(VkCommandBuffer cmdbuffer, std::string caption,
                   glm::vec4 color);
void cmdEndLabel(VkCommandBuffer cmdbuffer);
}  // namespace debugutils
}  // namespace vks