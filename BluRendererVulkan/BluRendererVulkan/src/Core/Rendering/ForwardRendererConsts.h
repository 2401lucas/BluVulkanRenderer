#ifndef FORWARDRENDERERCONSTS_H
#define FORWARDRENDERERCONSTS_H

#include <vulkan/vulkan.h>

constexpr uint32_t MAX_MODELS = 1000;
constexpr uint32_t MAX_VERTICES = 1000000;
constexpr uint32_t MAX_INDICES = 1000000;
constexpr uint32_t MAX_TEXTURES = 100;
constexpr uint32_t IMAGES_PER_FRAME = 2;
constexpr VkFormat DEPTH_FORMAT = VK_FORMAT_D32_SFLOAT;
constexpr VkFormat COLOR_FORMAT = VK_FORMAT_R8G8B8A8_UNORM;
constexpr VkDeviceSize MAX_BUFFERS_STORAGE = 32;
constexpr VkDeviceSize DRAW_COMMAND_BUFFER_SIZE =
    sizeof(VkDrawIndexedIndirectCommand);
#endif