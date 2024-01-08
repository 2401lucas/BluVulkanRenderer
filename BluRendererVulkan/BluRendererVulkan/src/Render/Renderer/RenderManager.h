#pragma once

#include "../Renderer/Renderer.h"

class RenderManager {
public:
	RenderManager(GLFWwindow* window, const VkApplicationInfo& appInfo, DeviceSettings deviceSettings);
	
	void createRenderer(const SceneDependancies& sceneDependancies);
	void drawFrame(const bool& framebufferResized, const SceneInfo* sceneInfo);

	void cleanup();

private:
	VulkanInstance* vkInstance;
	Device* device;

	Renderer* renderer;
};