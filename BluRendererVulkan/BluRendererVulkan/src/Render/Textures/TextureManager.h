#pragma once
#include "../Image/ImageUtils.h"
#include "../Image/Image.h"
#include "../src/Engine/Entity/Components/MaterialComponent.h"
#include <vector>

struct TextureData {
	Image* texture;	//PBR-> Albedo 
	Image* specular;//PBR-> Normal 
	Image* diffuse;	//PBR-> Metallic 
	Image* roughness;
	Image* ao;

	TextureData(Image* texture)
		: texture(texture) {}
	TextureData(Image* texture, Image* specular, Image* diffuse)
		: texture(texture), specular(specular), diffuse(diffuse) {}
	TextureData(Image* albedo, Image* normal, Image* metallic, Image* roughness, Image* ao)
		: texture(albedo), specular(normal), diffuse(metallic), roughness(roughness), ao(ao) {}

	void cleanup(Device* deviceInfo) {
		texture->cleanup(deviceInfo);
		delete(texture);

		if (specular != nullptr) {
			specular->cleanup(deviceInfo);
			delete(specular);
		}

		if (diffuse != nullptr) {
			diffuse->cleanup(deviceInfo);
			delete(diffuse);
		}

		if (roughness != nullptr) {
			roughness->cleanup(deviceInfo);
			delete(roughness);
		}

		if (ao != nullptr) {
			ao->cleanup(deviceInfo);
			delete(ao);
		}
	}
};

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