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
	Image* image6;

	TextureData() {}
	TextureData(Image* texture)
		: texture(texture) {}
	TextureData(Image* texture, Image* specular, Image* diffuse)
		: texture(texture), specular(specular), diffuse(diffuse) {}
	TextureData(Image* albedo, Image* normal, Image* metallic, Image* roughness, Image* ao)
		: texture(albedo), specular(normal), diffuse(metallic), roughness(roughness), ao(ao) {}
	TextureData(Image* albedo, Image* normal, Image* metallic, Image* roughness, Image* ao, Image* image6)
		: texture(albedo), specular(normal), diffuse(metallic), roughness(roughness), ao(ao), image6(image6) {}

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

		if (image6 != nullptr) {
			image6->cleanup(deviceInfo);
			delete(image6);
		}
	}
};

class TextureManager {
public:
	void cleanup(Device* deviceInfo);

	void loadTextures(Device* deviceInfo, CommandPool* commandPool, std::vector<TextureInfo> textureInfo);
	std::vector<TextureData>& getTextures();
	uint32_t getTextureIndex(TextureInfo info);
	uint32_t getTextureType(const char* path);

private:
	std::vector<TextureInfo> textureInfos;
	std::vector<TextureData> textures;
	TextureData skyBox;
};