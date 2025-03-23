#ifndef VKDEBUG_H
#define VKDEBUG_H

#include "vulkan/vulkan.h"

VKAPI_ATTR VkBool32 VKAPI_CALL debugUtilsMessageCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData);


#endif