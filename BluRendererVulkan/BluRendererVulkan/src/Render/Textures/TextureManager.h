#pragma once
#include <vector>

#include "../Image/Image.h"
#include "../Image/ImageUtils.h"
#include "../Materials/MaterialManager.h"

class TextureManager {
 public:
  void cleanup(Device* deviceInfo);

  void registerTexture(Device* deviceInfo, CommandPool* commandPool,
                       Texture& textureInfo);
  std::vector<Image*> getImages();
 private:
  std::unordered_map<std::string, uint32_t> texturesIndex;
  std::vector<Image*> images;
};