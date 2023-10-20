#include "CommandPool.h"
#include "../include/RenderConst.h"
#include <stdexcept>

CommandPool::CommandPool(Device* deviceInfo, const uint32_t familyQueue, VkCommandPoolCreateFlagBits flags)
{
	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.flags = flags;
	poolInfo.queueFamilyIndex = familyQueue;

	if (vkCreateCommandPool(deviceInfo->getLogicalDevice(), &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
		throw std::runtime_error("failed to create command pool!");
}

void CommandPool::cleanup(Device* deviceInfo)
{
	vkDestroyCommandPool(deviceInfo->getLogicalDevice(), commandPool, nullptr);
}

void CommandPool::allocCommandBuffer(Device* deviceInfo, VkCommandBuffer& commandBuffer) 
{
    VkCommandBufferAllocateInfo allocInfo = getBufferAllocInfo(1);

	vkAllocateCommandBuffers(deviceInfo->getLogicalDevice(), &allocInfo, &commandBuffer);
}

void CommandPool::beginCommandBuffer(VkCommandBuffer& commandBuffer, const VkCommandBufferUsageFlags& flags)
{
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = flags;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);
}

void CommandPool::submitBuffer(VkQueue& queue, VkCommandBuffer& commandBuffer)
{
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	//TODO: 8 USE FENCE
	if(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
		throw std::runtime_error("Failed to submit draw command buffer!");

	vkQueueWaitIdle(queue);
}

void CommandPool::endCommandBuffer(VkCommandBuffer& commandBuffer)
{
	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
		throw std::runtime_error("Failed to record command buffer!");
}

void CommandPool::freeCommandBuffer(Device* deviceInfo, VkCommandBuffer& commandBuffer)
{
	vkFreeCommandBuffers(deviceInfo->getLogicalDevice(), commandPool, 1, &commandBuffer);
}

void CommandPool::createCommandBuffers(Device* deviceInfo)
{
	commandBuffers.resize(RenderConst::MAX_FRAMES_IN_FLIGHT);

	VkCommandBufferAllocateInfo allocInfo = getBufferAllocInfo(commandBuffers.size());

	if (vkAllocateCommandBuffers(deviceInfo->getLogicalDevice(), &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate command buffers!");
	}
}

VkCommandBuffer& CommandPool::getCommandBuffer(uint32_t index)
{
	return commandBuffers[index];
}

VkCommandBufferAllocateInfo CommandPool::getBufferAllocInfo(uint32_t numOfBuffers)
{
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = commandPool;
	allocInfo.commandBufferCount = numOfBuffers;

	return allocInfo;
}