#pragma once
#include <vector>
#include "../Model/Model.h"
#include "../Image/Image.h"
#include "../Command/CommandPool.h"
#include "../Buffer/Buffer.h"
#include "../Camera/Camera.h"
#include "../Descriptors/Types/PushConsts/PushConst.h"
#include "../Buffer/MappedBufferManager.h"

class ModelManager {
public:
	ModelManager(Device* deviceInfo);
	void cleanup(Device* deviceInfo);

	void bindBuffers(const VkCommandBuffer& commandBuffer, const int32_t index);
	void updatePushConstants(VkCommandBuffer& commandBuffer, VkPipelineLayout& layout, const int32_t index);
	void drawIndexed(const VkCommandBuffer& commandBuffer);
	void loadModels(Device* deviceInfo, CommandPool* commandPool, const std::vector<SceneModel> modelCreateInfos);
	void loadTextures(Device* deviceInfo, CommandPool* commandPool, const std::vector<MaterialInfo>& materials);

	std::vector<Image*>& getTextures();
	uint32_t getTextureIndex(const char* path);

	void updateUniformBuffer(Device* deviceInfo, Camera* camera, const SceneInfo* sceneInfo, const uint32_t& mappedBufferManagerIndex, const uint32_t& bufferIndex);
	//Index 0 Global Resources and bound once per frame.
	//Index 1 Per-pass Resources, and bound once per pass
	//Index 2 Material Resources
	//Index 3 Per-Object Resources
	MappedBufferManager* getMappedBufferManager(uint32_t index);

private:
	void createVertexBuffer(Device* deviceInfo, CommandPool* commandPool, std::vector<Vertex> vertices);
	void createIndexBuffer(Device* deviceInfo, CommandPool* commandPool, std::vector<uint32_t> indices);
	
	std::vector<Buffer*> vertexBuffers;
	Buffer* indexBuffer;

	std::vector<Model*> models;
	std::vector<PushConstantData> modelData;
	
	std::vector<MaterialInfo> matInfos;
	std::vector<Image*> textures;

	MappedBufferManager* cameraMappedBufferManager;
	MappedBufferManager* sceneMappedBufferManager;
};