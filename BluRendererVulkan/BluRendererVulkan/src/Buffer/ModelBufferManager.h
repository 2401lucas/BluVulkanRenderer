#pragma once
#include <vector>
#include "../Model/Model.h"
#include "../Command/CommandPool.h"
#include "../Buffer/Buffer.h"


struct UniformBufferObject {
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
};

class ModelBufferManager {
public:
	ModelBufferManager(Device* deviceInfo, CommandPool* commandPool, std::vector<Model*> models);
	void cleanup(Device* deviceInfo);

	void UpdateUniformBuffer(Device* deviceInfo, VkExtent2D swapchainExtent, uint32_t index);
	std::vector<Buffer*> getUniformBuffers();
	Buffer* getUniformBuffer(uint32_t index);
	Model* getModels(uint32_t index);
	VkBuffer& getVertexBuffer();
	VkBuffer& getIndexBuffer();
	uint32_t getIndexSize();

private:
	void createVertexBuffer(Device* deviceInfo, CommandPool* commandPool, std::vector<Vertex> vertices);
	void createIndexBuffer(Device* deviceInfo, CommandPool* commandPool, std::vector<uint32_t> indices);
	void createUniformBuffer(Device* deviceInfo);

	Buffer* vertexBuffer;
	Buffer* indexBuffer;

	Model* model;

	std::vector<Buffer*> uniformBuffers;
	std::vector<void*> uniformBuffersMapped;
};