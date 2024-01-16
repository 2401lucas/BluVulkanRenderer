#include "TextureManager.h"


void TextureManager::cleanup(Device* deviceInfo) {

    for (auto texture : textures) {
        texture.cleanup(deviceInfo);
    }
}

void TextureManager::loadTextures(Device* deviceInfo, CommandPool* commandPool, const std::vector<TextureInfo>& textures) {
    textureInfos = textures;

    for (auto& tex : textures) {
        switch (tex.type) {
        case textureType::BASIC:
            this->textures.push_back(TextureData(ImageUtils::createImageFromPath(deviceInfo, commandPool, (tex.fileName + tex.fileType).c_str())));
            break;
        case textureType::DIFFSPEC:
            this->textures.push_back(TextureData(ImageUtils::createImageFromPath(deviceInfo, commandPool, (tex.fileName + tex.fileType).c_str()), ImageUtils::createImageFromPath(deviceInfo, commandPool, (tex.fileName + "_specular" + tex.fileType).c_str()), ImageUtils::createImageFromPath(deviceInfo, commandPool, (tex.fileName + "_diffuse" + tex.fileType).c_str())));
            break;
        case textureType::PBR:
            this->textures.push_back(TextureData(ImageUtils::createImageFromPath(deviceInfo, commandPool, (tex.fileName + tex.fileType).c_str()), ImageUtils::createImageFromPath(deviceInfo, commandPool, (tex.fileName + "_normal" + tex.fileType).c_str()), ImageUtils::createImageFromPath(deviceInfo, commandPool, (tex.fileName + "_metallic" + tex.fileType).c_str()), ImageUtils::createImageFromPath(deviceInfo, commandPool, (tex.fileName + "_roughness" + tex.fileType).c_str()), ImageUtils::createImageFromPath(deviceInfo, commandPool, (tex.fileName + "_ao" + tex.fileType).c_str())));
            break;
        }
    }
}

std::vector<TextureData>& TextureManager::getTextures() {
    return textures;
}

//TODO: Implement more optimized search like std::unordered_map with custom hash
uint32_t TextureManager::getTextureIndex(const char* path) {
    int index = 0;
    for (uint32_t i = 0; i < textureInfos.size(); i++) {
        if (textureInfos[i].fileName == path)
            return index;

        switch (textureInfos[i].type) {
        case textureType::BASIC:
            index += 1;
            break;
        case textureType::DIFFSPEC:
            index += 3;
            break;
        case textureType::PBR:
            index += 5;
            break;
        }
    }
    return 0;
}

uint32_t TextureManager::getTextureType(const char* path) {
    for (uint32_t i = 0; i < textureInfos.size(); i++) {
        if (textureInfos[i].fileName == path)
            return textureInfos[i].type;
    }
}