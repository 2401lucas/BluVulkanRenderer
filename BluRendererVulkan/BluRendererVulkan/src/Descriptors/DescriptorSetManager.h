#pragma once
#include "../Descriptors/DescriptorPool.h"
#include "../include/RenderConst.h"
#include "../Buffer/ModelBufferManager.h"
#include "Descriptor.h"

class DescriptorSetManager {
public:
	DescriptorSetManager(Device* deviceInfo, Descriptor* descriptorSetLayout, ModelBufferManager* modelManager);
	void cleanup(Device* deviceInfo);

	void createDescriptorSets(Device* deviceInfo, Descriptor* descriptorSetLayout, ModelBufferManager* modelManager);
	VkDescriptorSet* getDescriptorSet(uint32_t index);
private:
	DescriptorPool* descriptorPool;
	std::vector<VkDescriptorSet> descriptorSets;
};