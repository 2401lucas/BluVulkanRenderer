#pragma once
#include <vector>
#include "../Image/Image.h"
#include "../Command/CommandPool.h"
#include "../Buffer/Buffer.h"
#include "../Descriptors/Types/PushConsts/PushConst.h"
#include "../Buffer/MappedBufferManager.h"
#include "../Descriptors/Types/UBO/UBO.h"
#include "../Renderer/RenderSceneData.h"
#include "../../Engine/Mesh/MeshUtils.h"
#include "BufferAllocator.h"

class ModelBufferManager {
public:
	ModelBufferManager(Device* deviceInfo);
	void cleanup(Device* deviceInfo);

	std::pair<MemoryChunk, MemoryChunk> loadModelIntoBuffer(Device* device, CommandPool* commandPool, RenderModelCreateData modelData);
	void updateUniformBuffer(Device* deviceInfo, const uint32_t& bufferIndex, RenderSceneData& sceneData);
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

	BufferAllocator* vertexBufferAllocator;
	BufferAllocator* indexBufferAllocator;
};