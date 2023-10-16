#include "../Renderer/RenderManager.h"

RenderManager::RenderManager()
{
	vkInstance = std::make_unique<VulkanInstance>();
	device = std::make_unique<Device>(vkInstance);
	swapchain = std::make_unique<Swapchain>(WindowManager->getWindow(), device->GetPhysicalDevice(), WindowManager->getSurface());
}
