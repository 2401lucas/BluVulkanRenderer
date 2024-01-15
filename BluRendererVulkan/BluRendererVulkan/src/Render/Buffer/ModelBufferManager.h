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

class ModelBufferManager {
public:
	ModelBufferManager(Device* deviceInfo);
	void cleanup(Device* deviceInfo);

	void updateUniformBuffer(Device* deviceInfo, Camera* camera, const SceneInfo* sceneInfo, const uint32_t& bufferIndex, std::vector<Model*> models);
	void prepareBuffer(Device* deviceInfo, CommandPool* commandPool, std::vector<Vertex> vertices, std::vector<uint32_t> indices);
	void bindBuffers(const VkCommandBuffer& commandBuffer);
	void updatePushConstants(VkCommandBuffer& commandBuffer, VkPipelineLayout& layout, const PushConstantData& pushConstData);
	void drawIndexed(const VkCommandBuffer& commandBuffer, const int32_t& indexCount, const int32_t& vertexOffset, const int32_t& indexOffset);

	//Index 0 Camera
	//Index 1 Scene
	//Index 2 Material
	MappedBufferManager* getMappedBufferManager(uint32_t index);

private:
	MappedBufferManager* cameraMappedBufferManager;
	MappedBufferManager* sceneMappedBufferManager;
	MappedBufferManager* materialMappedBufferManager;

	Buffer* vertexBuffer;
	Buffer* indexBuffer;
};