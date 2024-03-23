#include "TextureLoader.h"

TextureLoader::TextureLoader() {}

void TextureLoader::destroyTexture(Texture& texture) {}

void TextureLoader::loadTexture(std::string filePath, VkFormat, Texture*) {}

void TextureLoader::loadTextureArray(std::string filePath, VkFormat, Texture*) {
}

void TextureLoader::loadCubemap(std::string filePath, VkFormat, Texture*) {}