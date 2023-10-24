#pragma once
#include "../Descriptors/DescriptorPool.h"
#include "../include/RenderConst.h"
#include "../Buffer/ModelManager.h"
#include "Descriptor.h"

class DescriptorSetManager {
public:
	DescriptorSetManager(Device* deviceInfo, Descriptor* descriptorSetLayout, ModelManager* modelManager);
	void cleanup(Device* deviceInfo);

	void createDescriptorSets(Device* deviceInfo, Descriptor* descriptorSetLayout, ModelManager* modelManager);
	VkDescriptorSet* getDescriptorSet(uint32_t index);
	VkDescriptorSet* getGlobalDescriptorSet();
private:
	DescriptorPool* descriptorPool;
	VkDescriptorSet globalDescriptorSet;
	std::vector<VkDescriptorSet> descriptorSets;
};