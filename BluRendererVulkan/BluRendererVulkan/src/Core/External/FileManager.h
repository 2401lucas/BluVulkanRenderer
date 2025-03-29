#ifndef FILEMANAGER_H
#define FILEMANAGER_H
#include <vulkan/vulkan.h>
#include <EASTL/string.h>

namespace blu::core ::file {
VkShaderModule LoadShader(const char* fileName, const VkDevice& device);

bool DoesFileExist(eastl::string fileName);
}
#endif