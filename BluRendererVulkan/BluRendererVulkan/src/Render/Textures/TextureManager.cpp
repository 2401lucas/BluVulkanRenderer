#include "TextureManager.h"

TextureManager::TextureManager() {}

void TextureManager::cleanup(Device* deviceInfo) {
  for (auto image : images) {
    image->cleanup(deviceInfo);
    delete (image);
  }

  textureIndices.clear();
  images.clear();
}

void TextureManager::registerTexture(Device* deviceInfo,
                                     CommandPool* commandPool,
                                     Texture& textureInfo) {
  if (textureIndices.count(textureInfo.filePath) == 0) {
    textureIndices[textureInfo.filePath] = images.size();
    textureInfo.imageIndex = images.size();
    images.push_back(ImageUtils::createImageFromPath(
        deviceInfo, commandPool, textureInfo.filePath.c_str()));
  }
}

std::vector<Image*> TextureManager::getImages() { return images; }