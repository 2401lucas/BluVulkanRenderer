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
#include "../src/Engine/Scene/Scene.h"
#include "../Camera/Camera.h"

class RenderManager {
public:
	RenderManager(VulkanInstance* vkInstance, Device* device, const SceneDependancies& sceneDependancies);
	void cleanup(Device* device);
	void drawFrame(Device* device, const bool& framebufferResized, const SceneInfo* sceneInfo, std::vector<Model*> models);

private:
	void createSyncObjects(Device* device);

	RenderPass* renderPass;
	Swapchain* swapchain;
	Descriptor* graphicsDescriptorSetLayout;
	Descriptor* graphicsMaterialDescriptorSetLayout;
	std::vector<GraphicsPipeline*> graphicsPipelines;
	CommandPool* graphicsCommandPool;
	ModelBufferManager* modelBufferManager;
	DescriptorSetManager* descriptorManager;
	Scene* currentScene;
	Camera* camera;

	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;
	uint32_t frameIndex;
};