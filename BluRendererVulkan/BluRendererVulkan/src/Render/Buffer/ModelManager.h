#pragma once
#include <vector>
#include "../Model/Model.h"
#include "../Image/Image.h"
#include "../Command/CommandPool.h"
#include "../Buffer/Buffer.h"
#include "../Camera/Camera.h"
#include "../Descriptors/Types/PushConsts/PushConst.h"
#include "../Buffer/MappedBufferManager.h"
#include "../Descriptors/Types/UBO/UBO.h"

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

class ModelManager {
public:
	ModelManager(Device* deviceInfo);
	void cleanup(Device* deviceInfo);

	void loadModels(Device* deviceInfo, CommandPool* commandPool, const std::vector<SceneModel> modelCreateInfos);
	void loadTextures(Device* deviceInfo, CommandPool* commandPool, const std::vector<TextureInfo>& textures);
	
	void bindBuffers(const VkCommandBuffer& commandBuffer, const int32_t index);
	void updatePushConstants(VkCommandBuffer& commandBuffer, VkPipelineLayout& layout, const int32_t index);
	void drawIndexed(const VkCommandBuffer& commandBuffer, const uint32_t& index);

	std::vector<TextureData>& getTextures();
	uint32_t getTextureIndex(const char* path);
	uint32_t getTextureType(const char* path);

	void updateUniformBuffer(Device* deviceInfo, Camera* camera, const SceneInfo* sceneInfo, const uint32_t& mappedBufferManagerIndex, const uint32_t& bufferIndex);
	//Index 0 Camera.
	//Index 1 Scene
	//Index 2 Material
	MappedBufferManager* getMappedBufferManager(uint32_t index);
	int getModelCount();

private:
	void createVertexBuffer(Device* deviceInfo, CommandPool* commandPool, std::vector<Vertex> vertices);
	void createIndexBuffer(Device* deviceInfo, CommandPool* commandPool, std::vector<uint32_t> indices);

	std::vector<Buffer*> vertexBuffers;
	std::vector<Buffer*> indexBuffers;

	std::vector<Model*> models;
	std::vector<PushConstantData> modelData;

	std::vector<TextureInfo> textureInfos;
	std::vector<TextureData> textures;

	MappedBufferManager* cameraMappedBufferManager;
	MappedBufferManager* sceneMappedBufferManager;
	MappedBufferManager* materialMappedBufferManager;
};