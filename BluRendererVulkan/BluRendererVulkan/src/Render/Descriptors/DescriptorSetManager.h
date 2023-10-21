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
private:
	DescriptorPool* descriptorPool;
	std::vector<VkDescriptorSet> descriptorSets;
};