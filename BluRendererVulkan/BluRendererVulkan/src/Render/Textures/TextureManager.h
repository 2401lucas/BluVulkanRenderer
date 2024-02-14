#pragma once
#include <unordered_map>
#include <vector>

#include "../Image/Image.h"
#include "../Image/ImageUtils.h"

struct Texture {
  std::string filePath;
  uint32_t imageIndex;
};

class TextureManager {
 public:
  TextureManager();

  void cleanup(Device* deviceInfo);

  void registerTexture(Device* deviceInfo, CommandPool* commandPool,
                       Texture& textureInfo);
  std::vector<Image*> getImages();

 private:
  std::unordered_map<std::string, uint32_t> textureIndices;
  std::vector<Image*> images;
};