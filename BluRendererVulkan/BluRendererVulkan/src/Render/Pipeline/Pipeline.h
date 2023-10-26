#pragma once

#include <string>
#include "../src/Engine/FileManagement/FileManager.h"
#include "../Device/Device.h"
#include "../../Engine/Scene/Scene.h"

class Pipeline {
public: 
	Pipeline();
	virtual void cleanup(Device* deviceInfo);

	void bindPipeline(const VkCommandBuffer& commandBuffer, const VkPipelineBindPoint& pipelineBindpoint);
	void bindDescriptorSets(const VkCommandBuffer& commandBuffer, const VkPipelineBindPoint& pipelineBindpoint, const uint32_t& firstSet, const uint32_t& descriptorSetCount, const VkDescriptorSet* descriptorSets, const uint32_t& dynamicOffsetCount, const uint32_t* dynamicOffsets);

	VkPipeline& getPipeline();
	VkPipelineLayout& getPipelineLayout();
protected:
	VkShaderModule createShaderModule(Device* deviceInfo, const ShaderInfo& shaderInfo);
	std::vector<char> getBinaryData(const ShaderInfo& shaderInfo);
	std::vector<VkPipelineShaderStageCreateInfo> getShaderStageInfo();
	void freeShaderModules(Device* deviceInfo);

	VkPipelineLayout pipelineLayout;
	VkPipeline pipeline;

private:
	VkPipelineShaderStageCreateInfo createShaderStageInfo(VkShaderModule shaderModule, VkShaderStageFlagBits flag);

	std::vector<VkShaderModule> vertShaderModules;
	std::vector<VkShaderModule> fragShaderModules;
	std::vector<VkShaderModule> compShaderModules;
};