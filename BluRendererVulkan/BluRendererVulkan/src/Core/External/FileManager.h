#ifndef FILEMANAGER_H
#define FILEMANAGER_H
#include <vulkan/vulkan.h>

namespace blu::core ::file {
VkShaderModule LoadShader(const char* fileName, const VkDevice& device);

}
#endif