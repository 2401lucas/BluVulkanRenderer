#pragma once
#include "../Descriptors/DescriptorPool.h"
#include "../include/RenderConst.h"
#include "../Buffer/ModelManager.h"
#include "Descriptor.h"

class DescriptorSetManager {
public:
	DescriptorSetManager(Device* deviceInfo, const std::vector<VkDescriptorSetLayout>& descriptorLayouts, ModelManager* modelManager);
	void cleanup(Device* deviceInfo);

	void createDescriptorSets(Device* deviceInfo, const std::vector<VkDescriptorSetLayout>& descriptorLayouts, ModelManager* modelManager);
	VkDescriptorSet* getGlobalDescriptorSet(uint32_t index);
	VkDescriptorSet* getMaterialDescriptorSet(uint32_t index);
private:
	DescriptorPool* globalDescriptorPool;
	DescriptorPool* matDescriptorPool;
	std::vector<VkDescriptorSet> globalDescriptorSets;
	std::vector<VkDescriptorSet> materialDescriptorSets;
};