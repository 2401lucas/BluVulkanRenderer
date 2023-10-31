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

class ModelManager {
public:
	ModelManager(Device* deviceInfo);
	void cleanup(Device* deviceInfo);

	void bindBuffers(const VkCommandBuffer& commandBuffer, const int32_t index);
	void updatePushConstants(VkCommandBuffer& commandBuffer, VkPipelineLayout& layout, const int32_t index);
	void drawIndexed(const VkCommandBuffer& commandBuffer, const uint32_t& index);
	void loadModels(Device* deviceInfo, CommandPool* commandPool, const std::vector<SceneModel> modelCreateInfos);
	void loadTextures(Device* deviceInfo, CommandPool* commandPool, const std::vector<TextureInfo>& textures);
	void loadMaterials(Device* deviceInfo, CommandPool* commandPool, const std::vector<MaterialInfo>& materials);

	std::vector<Image*>& getTextures();
	uint32_t getTextureIndex(const char* path);

	GPUMaterialData& getMaterials();
	uint32_t getMaterialIndex();

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
	std::vector<Image*> textures;

	std::vector<MaterialInfo> materialInfos;
	GPUMaterialData mat;

	MappedBufferManager* cameraMappedBufferManager;
	MappedBufferManager* sceneMappedBufferManager;
	MappedBufferManager* materialMappedBufferManager;
};