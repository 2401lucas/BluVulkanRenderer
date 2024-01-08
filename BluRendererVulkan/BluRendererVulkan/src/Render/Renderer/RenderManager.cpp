#include "RenderManager.h"

RenderManager::RenderManager(GLFWwindow* window, const VkApplicationInfo& appInfo, DeviceSettings deviceSettings)
{
	vkInstance = new VulkanInstance(appInfo);
	device = new Device(window, vkInstance, deviceSettings);
	renderer = nullptr;
}

void RenderManager::createRenderer(const SceneDependancies& sceneDependancies)
{
	if (renderer != nullptr) {
		renderer->cleanup(device);
		delete renderer;
	}

	renderer = new Renderer(vkInstance, device, sceneDependancies);
}

void RenderManager::drawFrame(const bool& framebufferResized, const SceneInfo* sceneInfo)
{
	if (renderer == nullptr) {
		throw std::runtime_error("Trying to draw frame, but Renderer is null");
		return;
	}

	renderer->drawFrame(device, framebufferResized, sceneInfo);
}

void RenderManager::cleanup()
{
	if (renderer != nullptr)
	{
		renderer->cleanup(device);
		delete renderer;
	}

	device->cleanup(vkInstance);
	delete device;
	vkInstance->cleanup();
	delete vkInstance;
}