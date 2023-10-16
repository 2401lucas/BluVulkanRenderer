#include <iostream>

#include "../VkInstance/VulkanInstance.h"
#include "../Device/Device.h"
#include "../Swapchain/Swapchain.h"

class RenderManager {
public:
	RenderManager();
	~RenderManager();
private:
	std::unique_ptr<VulkanInstance> vkInstance;
	std::unique_ptr<Device> device;
	std::unique_ptr<Swapchain> swapchain;
};