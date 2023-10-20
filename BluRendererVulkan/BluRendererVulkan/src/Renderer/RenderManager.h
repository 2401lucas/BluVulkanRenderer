#pragma once

#include <iostream>

#include "../VkInstance/VulkanInstance.h"
#include "../Device/Device.h"
#include "../RenderPass/RenderPass.h"
#include "../Swapchain/Swapchain.h"
#include "../Descriptors/Descriptor.h"
#include "../Pipeline/GraphicsPipeline.h"
#include "../Command/CommandPool.h"
#include "../Model/Model.h"
#include "../Buffer/Buffer.h"
#include "../Buffer/ModelBufferManager.h"
#include "../Descriptors/DescriptorSetManager.h"

class RenderManager {
public:
	RenderManager(GLFWwindow* window, const VkApplicationInfo& appInfo, DeviceSettings deviceSettings);
	void cleanup();
	void drawFrame();

private:
	void createSyncObjects();

	VulkanInstance* vkInstance;
	Device* device;
	RenderPass* renderPass;
	Swapchain* swapchain;
	Descriptor* graphicsDescriptorSetLayout;
	GraphicsPipeline* graphicsPipeline;
	CommandPool* graphicsCommandPool;
	ModelBufferManager* modelManager;
	DescriptorSetManager* descriptorManager;

	std::vector<ShaderInfo> shaders;

	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;
	uint32_t currentFrame;
};