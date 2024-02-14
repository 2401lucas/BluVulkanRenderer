#include "TextureManager.h"

void TextureManager::cleanup(Device* deviceInfo) {
  for (auto image : images) {
    image->cleanup(deviceInfo);
    delete (image);
  }

  texturesIndex.clear();
  images.clear();
}

void TextureManager::registerTexture(Device* deviceInfo,
                                     CommandPool* commandPool,
                                     Texture& textureInfo) {
  if (texturesIndex.count(textureInfo.filePath) == 0) {
    texturesIndex[textureInfo.filePath] = images.size();
    textureInfo.imageIndex = images.size();
    images.push_back(ImageUtils::createImageFromPath(
        deviceInfo, commandPool, textureInfo.filePath.c_str()));
  }
}

std::vector<Image*> TextureManager::getImages() { return images; }