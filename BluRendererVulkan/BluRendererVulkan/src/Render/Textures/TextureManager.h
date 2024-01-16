#pragma once
#include "../Image/ImageUtils.h"
#include "../Model/ModelManager.h"
#include <vector>

class TextureManager {
public:
	void cleanup(Device* deviceInfo);

	void loadTextures(Device* deviceInfo, CommandPool* commandPool, const std::vector<TextureInfo>& textures);
	std::vector<TextureData>& getTextures();
	uint32_t getTextureIndex(const char* path);
	uint32_t getTextureType(const char* path);

private:
	std::vector<TextureInfo> textureInfos;
	std::vector<TextureData> textures;
};