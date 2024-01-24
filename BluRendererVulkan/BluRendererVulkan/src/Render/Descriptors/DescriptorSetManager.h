#pragma once
#include "../Descriptors/DescriptorPool.h"
#include "../include/RenderConst.h"
#include "Descriptor.h"
#include "../src/Render/Buffer/ModelBufferManager.h"
#include "../Textures/TextureManager.h"

class DescriptorSetManager {
public:
	DescriptorSetManager(Device* deviceInfo, const std::vector<VkDescriptorSetLayout>& descriptorLayouts, ModelBufferManager* modelBufferManager, TextureManager& textureManager);
	void cleanup(Device* deviceInfo);

	void createDescriptorSets(Device* deviceInfo, const std::vector<VkDescriptorSetLayout>& descriptorLayouts, ModelBufferManager* modelBufferManager, TextureManager& textureManager);
	VkDescriptorSet* getGlobalDescriptorSet(uint32_t index);
	VkDescriptorSet* getMaterialDescriptorSet(uint32_t index);
private:
	DescriptorPool* globalDescriptorPool;
	DescriptorPool* matDescriptorPool;
	std::vector<VkDescriptorSet> globalDescriptorSets;
	std::vector<VkDescriptorSet> materialDescriptorSets;
};