#include "FileManager.h"

#include <cassert>
#include <fstream>
#include <iostream>

VkShaderModule blu::core::file::LoadShader(const char* fileName,
                                           const VkDevice& device) {
  std::ifstream is(fileName, std::ios::binary | std::ios::in | std::ios::ate);

  if (is.is_open() && is.good()) {
    size_t size = is.tellg();
    is.seekg(0, std::ios::beg);
    char* shaderCode = new char[size];
    is.read(shaderCode, size);
    is.close();

    assert(size > 0);

    VkShaderModule shaderModule;
    VkShaderModuleCreateInfo moduleCreateInfo{};
    moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    moduleCreateInfo.codeSize = size;
    moduleCreateInfo.pCode = (uint32_t*)shaderCode;

    assert(vkCreateShaderModule(device, &moduleCreateInfo, NULL,
                                &shaderModule) == VK_SUCCESS);

    delete[] shaderCode;

    return shaderModule;
  } else {
    std::cerr << "Error: Could not open shader file \"" << fileName << "\""
              << "\n";
    return VK_NULL_HANDLE;
  }
}

bool blu::core::file::DoesFileExist(eastl::string fileName) {
  std::ifstream file(fileName.c_str());
  return file.good();
}