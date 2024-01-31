#pragma once

#include <iostream>

#include "../VkInstance/VulkanInstance.h"
#include "../Device/Device.h"
#include "../RenderPass/RenderPass.h"
#include "../Swapchain/Swapchain.h"
#include "../Descriptors/Descriptor.h"
#include "../Pipeline/GraphicsPipeline.h"
#include "../Command/CommandPool.h"
#include "../Buffer/Buffer.h"
#include "../Buffer/ModelBufferManager.h"
#include "../Descriptors/DescriptorSetManager.h"
#include "RenderSceneData.h"
#include "../Textures/TextureManager.h"


class RenderManager {
public:
	RenderManager(GLFWwindow* window, const VkApplicationInfo& appInfo, DeviceSettings deviceSettings, const SceneDependancies& sceneDependancies);
	void cleanup();

	void drawFrame(const bool& framebufferResized, RenderSceneData& sceneData);

	std::pair<MemoryChunk, MemoryChunk> registerMesh(RenderModelCreateData modelCreateInfo);

private:
	void createSyncObjects();

	VulkanInstance* vkInstance;
	Device* device;
	RenderPass* renderPass;
	Swapchain* swapchain;
	Descriptor* graphicsDescriptorSetLayout;
	Descriptor* graphicsMaterialDescriptorSetLayout;
	std::vector<GraphicsPipeline*> graphicsPipelines;
	CommandPool* graphicsCommandPool;
	ModelBufferManager* modelBufferManager;
	DescriptorSetManager* descriptorManager;

	TextureManager textureManager;

	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;
	uint32_t frameIndex;
};