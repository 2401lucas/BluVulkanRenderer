#include "Pipeline.h"
#include <stdexcept>

Pipeline::Pipeline()
{
	
}

void Pipeline::cleanup(Device* deviceInfo)
{
	vkDestroyPipeline(deviceInfo->getLogicalDevice(), pipeline, nullptr);
	vkDestroyPipelineLayout(deviceInfo->getLogicalDevice(), pipelineLayout, nullptr);
}

void Pipeline::bindPipeline(const VkCommandBuffer& commandBuffer, const VkPipelineBindPoint& pipelineBindpoint)
{
	vkCmdBindPipeline(commandBuffer, pipelineBindpoint, pipeline);
}

void Pipeline::bindDescriptorSets(const VkCommandBuffer& commandBuffer, const VkPipelineBindPoint& pipelineBindpoint, const uint32_t& firstSet, const uint32_t& descriptorSetCount, const VkDescriptorSet* descriptorSets, const uint32_t& dynamicOffsetCount, const uint32_t* dynamicOffsets)
{
	vkCmdBindDescriptorSets(commandBuffer, pipelineBindpoint, pipelineLayout, firstSet, descriptorSetCount, descriptorSets, dynamicOffsetCount, dynamicOffsets);
}

VkPipeline& Pipeline::getPipeline()
{
	return pipeline;
}

VkPipelineLayout& Pipeline::getPipelineLayout()
{
	return pipelineLayout;
}

VkShaderModule Pipeline::createShaderModule(Device* deviceInfo, const ShaderInfo& shaderInfo)
{
	auto code = getBinaryData(shaderInfo);

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(deviceInfo->getLogicalDevice(), &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shader module!");
    }
	
	switch (shaderInfo.type)
	{
	case VERTEX:
		vertShaderModules.push_back(shaderModule);
		break;
	case FRAGMENT:
		fragShaderModules.push_back(shaderModule);
		break;
	case COMPUTE:
		compShaderModules.push_back(shaderModule);
		break;
	}
	return nullptr;
}

std::vector<char> Pipeline::getBinaryData(const ShaderInfo& shaderInfo)
{
	std::vector<char> code;

	switch (shaderInfo.type)
	{
	case VERTEX:
		code = FileManager::readBinary(("shaders/" + shaderInfo.fileName).c_str());
		break;
	case FRAGMENT:
		code = FileManager::readBinary(("shaders/" + shaderInfo.fileName).c_str());
		break;
	case COMPUTE:
		code = FileManager::readBinary(("shaders/" + shaderInfo.fileName).c_str());
		break;
	case NONE:
	default:
		break;
	}

	return code;
}

std::vector<VkPipelineShaderStageCreateInfo> Pipeline::getShaderStageInfo()
{
	std::vector<VkPipelineShaderStageCreateInfo> shaderStageCreateInfo;

	for (auto vertShaderModule : vertShaderModules) {
		VkPipelineShaderStageCreateInfo vertShaderStageInfo = createShaderStageInfo(vertShaderModule, VK_SHADER_STAGE_VERTEX_BIT);
		shaderStageCreateInfo.push_back(vertShaderStageInfo);
	}

	for (auto fragShaderModule : fragShaderModules) {
		VkPipelineShaderStageCreateInfo fragShaderStageInfo = createShaderStageInfo(fragShaderModule, VK_SHADER_STAGE_FRAGMENT_BIT);
		shaderStageCreateInfo.push_back(fragShaderStageInfo);
	}

	for (auto compShaderModule : compShaderModules) {
		VkPipelineShaderStageCreateInfo compShaderStageInfo = createShaderStageInfo(compShaderModule, VK_SHADER_STAGE_COMPUTE_BIT);
		shaderStageCreateInfo.push_back(compShaderStageInfo);
	}

	return shaderStageCreateInfo;
}

VkPipelineShaderStageCreateInfo Pipeline::createShaderStageInfo(VkShaderModule shaderModule, VkShaderStageFlagBits flag)
{
	VkPipelineShaderStageCreateInfo shaderStageInfo{};
	shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStageInfo.stage = flag;
	shaderStageInfo.module = shaderModule;
	shaderStageInfo.pName = "main";
	return shaderStageInfo;
}

void Pipeline::freeShaderModules(Device* deviceInfo)
{
	for (auto vertShaderModule : vertShaderModules) {
		vkDestroyShaderModule(deviceInfo->getLogicalDevice(), vertShaderModule, nullptr);
	}

	for (auto fragShaderModule : fragShaderModules) {
		vkDestroyShaderModule(deviceInfo->getLogicalDevice(), fragShaderModule, nullptr);
	}

	for (auto compShaderModule : compShaderModules) {
		vkDestroyShaderModule(deviceInfo->getLogicalDevice(), compShaderModule, nullptr);
	}
}