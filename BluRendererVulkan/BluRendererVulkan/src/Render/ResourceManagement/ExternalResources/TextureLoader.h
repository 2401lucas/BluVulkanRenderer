#pragma once
#include <vulkan/vulkan_core.h>

#include <string>

#include "Texture.h"

class TextureLoader {
 public:
  TextureLoader();

  void destroyTexture(Texture& texture);
  void loadTexture(std::string filePath, VkFormat, Texture*);
  void loadTextureArray(std::string filePath, VkFormat, Texture*);
  void loadCubemap(std::string filePath, VkFormat, Texture*);

  private: 
  VkDevice* device;
};