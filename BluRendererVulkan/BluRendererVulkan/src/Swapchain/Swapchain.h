#pragma once

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <vector>
#include "../Image/Image.h"
#include "../Device/Device.h"
#include "../RenderPass/RenderPass.h"


class Swapchain {
	struct SwapChainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

public:
	Swapchain(Device* deviceInfo);
	void cleanup(Device* deviceInfo);

	void createFramebuffers(Device* deviceInfo, RenderPass* renderPass);
	void reCreateSwapchain(Device* deviceInfo, RenderPass* renderPass);
	VkFormat getSwapchainFormat();
	VkSwapchainKHR getSwapchain();
	VkExtent2D getSwapchainExtent();
	VkFramebuffer& getFramebuffer(uint32_t index);
private:
	void createSwapchain(Device*);
	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	VkExtent2D chooseSwapExtent(Device* deviceInfo, const VkSurfaceCapabilitiesKHR& capabilities);

	void createImageViews(Device* deviceInfo);
	void createColorResources(Device* deviceInfo);
	void createDepthResources(Device* deviceInfo);
	VkFormat findDepthFormat(Device* deviceInfo);

	VkSwapchainKHR swapchain;
	std::vector<VkImage> swapchainImages;
	std::vector<VkImageView> swapchainImageViews;
	VkFormat swapchainImageFormat;
	VkExtent2D swapchainExtent;
	
	Image* colorImage;
	Image* depthImage;

	std::vector<VkFramebuffer> swapchainFramebuffers;
};