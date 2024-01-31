#include "TextureManager.h"

void TextureManager::cleanup(Device* deviceInfo) {

    for (auto texture : textures) {
        texture.cleanup(deviceInfo);
    }
}

void TextureManager::loadTextures(Device* deviceInfo, CommandPool* commandPool, std::vector<TextureInfo> textureInfo) {
    textureInfos = textureInfo;
    
    for (auto& tex : textureInfo) {
        switch (tex.type) {
        case TextureType::SingleTexture:
            this->textures.push_back(TextureData(ImageUtils::createImageFromPath(deviceInfo, commandPool, (tex.fileName + tex.fileType).c_str())));
            break;
        case TextureType::Phong:
            this->textures.push_back(TextureData(ImageUtils::createImageFromPath(deviceInfo, commandPool, (tex.fileName + tex.fileType).c_str()), ImageUtils::createImageFromPath(deviceInfo, commandPool, (tex.fileName + "_specular" + tex.fileType).c_str()), ImageUtils::createImageFromPath(deviceInfo, commandPool, (tex.fileName + "_diffuse" + tex.fileType).c_str())));
            break;
        case TextureType::PBR:
            this->textures.push_back(TextureData(ImageUtils::createImageFromPath(deviceInfo, commandPool, (tex.fileName + tex.fileType).c_str()), ImageUtils::createImageFromPath(deviceInfo, commandPool, (tex.fileName + "_normal" + tex.fileType).c_str()), ImageUtils::createImageFromPath(deviceInfo, commandPool, (tex.fileName + "_metallic" + tex.fileType).c_str()), ImageUtils::createImageFromPath(deviceInfo, commandPool, (tex.fileName + "_roughness" + tex.fileType).c_str()), ImageUtils::createImageFromPath(deviceInfo, commandPool, (tex.fileName + "_ao" + tex.fileType).c_str())));
            break;
        case TextureType::Cubemap:
            skyBox = TextureData(ImageUtils::createCubemapFromPath(deviceInfo, commandPool, tex.fileName, tex.fileType));
            break;
        }
    }
}

std::vector<TextureData>& TextureManager::getTextures() {
    return textures;
}

//TODO: Implement more optimized search like std::unordered_map with custom hash
uint32_t TextureManager::getTextureIndex(TextureInfo info) {
    int index = 0;
    for (uint32_t i = 0; i < textureInfos.size(); i++) {
        if (textureInfos[i].fileName == info.fileName)
            return index;

        switch (textureInfos[i].type) {
        case TextureType::SingleTexture:
            index += 1;
            break;
        case TextureType::Phong:
            index += 3;
            break;
        case TextureType::PBR:
            index += 5;
            break;
        }
    }
    return -1;
}

uint32_t TextureManager::getTextureType(const char* path) {
    for (uint32_t i = 0; i < textureInfos.size(); i++) {
        if (textureInfos[i].fileName == path)
            return textureInfos[i].type;
    }
    return 0;
}