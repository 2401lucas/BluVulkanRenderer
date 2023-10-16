#pragma once
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <vector>
#include "../Image/Image.h"


class Swapchain {
	struct SwapChainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

public:
	Swapchain(GLFWwindow*, VkPhysicalDevice, VkDevice, VkSurfaceKHR);
	~Swapchain();

	void reCreateSwapchain(GLFWwindow*, VkPhysicalDevice, VkDevice, VkSurfaceKHR);
private:
	void createSwapchain(GLFWwindow*, VkPhysicalDevice, VkDevice, VkSurfaceKHR);
	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice);
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>&);
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>&);
	VkExtent2D chooseSwapExtent(GLFWwindow*, const VkSurfaceCapabilitiesKHR&);

	void createImageViews();
	VkImageView createImageView(VkDevice, VkImage, VkFormat, VkImageAspectFlags, uint32_t);
	void createColorResources(VkPhysicalDevice, VkDevice);

	VkSwapchainKHR swapChain;
	std::vector<VkImage> swapChainImages;
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;
	std::vector<VkImageView> swapChainImageViews;

	Image* colorImage;
};