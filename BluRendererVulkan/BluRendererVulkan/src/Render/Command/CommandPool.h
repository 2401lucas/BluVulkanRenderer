#pragma once

#include "../Device/Device.h"

class CommandPool {
public:
	CommandPool(Device* deviceInfo, const uint32_t familyQueue, VkCommandPoolCreateFlagBits flags);
	void cleanup(Device* deviceInfo);

	void allocCommandBuffer(Device* deviceInfo, VkCommandBuffer& commandBuffer);
	void beginCommandBuffer(VkCommandBuffer& commandBuffer, const VkCommandBufferUsageFlags& flags);
	void submitBuffer(VkQueue& queue, VkCommandBuffer& commandBuffers);
	void endCommandBuffer(VkCommandBuffer& commandBuffer);
	void freeCommandBuffer(Device* deviceInfo, VkCommandBuffer& commandBuffer);
	void createCommandBuffers(Device* deviceInfo);
	VkCommandBuffer& getCommandBuffer(uint32_t index);
private:
	VkCommandBufferAllocateInfo getBufferAllocInfo(uint32_t numOfBuffers);

	VkCommandPool commandPool;

	// TODO: THIS SHOULD NOT EXIST IN THE COMMAND POOL
	std::vector<VkCommandBuffer> commandBuffers;
};