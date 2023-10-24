#pragma once
#include <vector>
#include "../Model/Model.h"
#include "../Command/CommandPool.h"
#include "../Buffer/Buffer.h"
#include "../Camera/Camera.h"
#include "../Descriptors/Types/PushConsts/PushConst.h"
#include "../Buffer/MappedBufferManager.h"

class ModelManager {
public:
	ModelManager(Device* deviceInfo, CommandPool* commandPool, const std::vector<ModelCreateInfo> modelCreateInfos);
	void cleanup(Device* deviceInfo);

	void bindBuffers(const VkCommandBuffer& commandBuffer);
	void updatePushConstants(VkCommandBuffer& commandBuffer, VkPipelineLayout& layout);
	void drawIndexed(const VkCommandBuffer& commandBuffer);


	void updateUniformBuffer(Device* deviceInfo, Camera* camera, const uint32_t& mappedBufferManagerIndex, const uint32_t& bufferIndex);
	//Index 0 Global Resources and bound once per frame.
	//Index 1 Per-pass Resources, and bound once per pass
	//Index 2 Material Resources
	//Index 3 Per-Object Resources
	MappedBufferManager* getMappedBufferManager(uint32_t index);
	Model* getModel(uint32_t index);
	VkBuffer& getVertexBuffer();
	VkBuffer& getIndexBuffer();
	uint32_t getIndexSize();

private:
	void createVertexBuffer(Device* deviceInfo, CommandPool* commandPool, std::vector<Vertex> vertices);
	void createIndexBuffer(Device* deviceInfo, CommandPool* commandPool, std::vector<uint32_t> indices);
	
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

	Buffer* vertexBuffer;
	Buffer* indexBuffer;

	std::vector<Model*> models;
	std::vector<PushConstantData> modelData;
	
	MappedBufferManager* cameraMappedBufferManager;
	MappedBufferManager* sceneMappedBufferManager;
};