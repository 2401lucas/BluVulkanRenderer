#include "Swapchain.h"
#include <stdexcept>
#include <algorithm>
#include <array>
#include <iostream>

Swapchain::Swapchain(Device* deviceInfo)
{
	createSwapchain(deviceInfo);
	createImageViews(deviceInfo);
	createColorResources(deviceInfo);
	createDepthResources(deviceInfo);
}

void Swapchain::cleanup(Device* deviceInfo)
{
	depthImage->cleanup(deviceInfo);
	delete depthImage;
	colorImage->cleanup(deviceInfo);
	delete colorImage;

	for (auto framebuffer : swapchainFramebuffers) {
		vkDestroyFramebuffer(deviceInfo->getLogicalDevice(), framebuffer, nullptr);
	}
	for (auto imageView : swapchainImageViews) {
		vkDestroyImageView(deviceInfo->getLogicalDevice(), imageView, nullptr);
	}
	vkDestroySwapchainKHR(deviceInfo->getLogicalDevice(), swapchain, nullptr);
}

void Swapchain::reCreateSwapchain(Device* deviceInfo, RenderPass* renderPass)
{
	vkDeviceWaitIdle(deviceInfo->getLogicalDevice());

	cleanup(deviceInfo);

	createSwapchain(deviceInfo);
	createImageViews(deviceInfo);
	createColorResources(deviceInfo);
	createDepthResources(deviceInfo);
	createFramebuffers(deviceInfo, renderPass);
}

VkFormat Swapchain::getSwapchainFormat()
{
	return swapchainImageFormat;
}

VkSwapchainKHR Swapchain::getSwapchain()
{
	return swapchain;
}

VkExtent2D Swapchain::getSwapchainExtent()
{
	return swapchainExtent;
}

VkFramebuffer& Swapchain::getFramebuffer(uint32_t index)
{
	return swapchainFramebuffers[index];
}

void Swapchain::createSwapchain(Device* deviceInfo)
{
	SwapChainSupportDetails swapChainSupport = querySwapChainSupport(deviceInfo->getPhysicalDevice(), deviceInfo->getSurface());
	VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
	VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
	VkExtent2D extent = chooseSwapExtent(deviceInfo, swapChainSupport.capabilities);

	uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
	if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
		imageCount = swapChainSupport.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = deviceInfo->getSurface();
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // TODO: 9 VK_IMAGE_USAGE_TRANSFER_DST_BIT for post processing

	QueueFamilyIndices indices = deviceInfo->findQueueFamilies();
	uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };
	if (indices.graphicsFamily != indices.presentFamily) {
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else { //TODO: 8 REMOVE VK_SHARING_MODE_CONCURRENT option for performance and do ownership transfer
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0;
		createInfo.pQueueFamilyIndices = nullptr;
	}

	createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = VK_NULL_HANDLE;

	if (vkCreateSwapchainKHR(deviceInfo->getLogicalDevice(), &createInfo, nullptr, &swapchain) != VK_SUCCESS) {
		throw std::runtime_error("failed to create swap chain!");
	}

	vkGetSwapchainImagesKHR(deviceInfo->getLogicalDevice(), swapchain, &imageCount, nullptr);
	swapchainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(deviceInfo->getLogicalDevice(), swapchain, &imageCount, swapchainImages.data());
	swapchainImageFormat = surfaceFormat.format;
	swapchainExtent = extent;
}

Swapchain::SwapChainSupportDetails Swapchain::querySwapChainSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
{
	SwapChainSupportDetails details;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &details.capabilities);

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
	if (formatCount != 0) {
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, details.formats.data());
	}

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);
	if (presentModeCount != 0) {
		details.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, details.presentModes.data());
	}

	return details;
}

VkSurfaceFormatKHR Swapchain::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
	for (const auto& availableFormat : availableFormats) {
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			return availableFormat;
		}
	}
	//TODO: 7 Return rank and return best from available formats
	return availableFormats[0];
}

VkPresentModeKHR Swapchain::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
	// Prefer VK_PRESENT_MODE_MAILBOX_KHR on pc
	// TODO: NA Prefer VK_PRESENT_MODE_FIFO_KHR on mobile due to higher energy usage

	for (const auto& availablePresentMode : availablePresentModes) {
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
			return availablePresentMode;
		}
	}

	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Swapchain::chooseSwapExtent(Device* deviceInfo, const VkSurfaceCapabilitiesKHR& capabilities)
{
	// TODO: 3 Investigate return capabilities.currentExtent;
	int width, height;
	glfwGetFramebufferSize(deviceInfo->getWindow(), &width, &height);
	VkExtent2D actualExtent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };

	//TODO: 8 USE OWN MATH LIBRARY
	actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
	actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

	return actualExtent;
}

void Swapchain::createImageViews(Device* deviceInfo)
{
	swapchainImageViews.resize(swapchainImages.size());

	for (uint32_t i = 0; i < swapchainImages.size(); i++) {
		swapchainImageViews[i] = Image::createImageView(deviceInfo, swapchainImages[i], swapchainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, (uint32_t)1);
	}
}

void Swapchain::createColorResources(Device* deviceInfo)
{
	VkFormat colorFormat = swapchainImageFormat;
	colorImage = new Image(deviceInfo, swapchainExtent.width, swapchainExtent.height, 1, colorFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
}

void Swapchain::createFramebuffers(Device* deviceInfo, RenderPass* renderPass)
{
	swapchainFramebuffers.resize(swapchainImages.size());
	for (size_t i = 0; i < swapchainImages.size(); i++) {
		std::array<VkImageView, 3> attachments = { colorImage->getImageView() , depthImage->getImageView(), swapchainImageViews[i]};
		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = renderPass->getRenderPass();
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = swapchainExtent.width;
		framebufferInfo.height = swapchainExtent.height;
		framebufferInfo.layers = 1;
		if (vkCreateFramebuffer(deviceInfo->getLogicalDevice(), &framebufferInfo, nullptr, &swapchainFramebuffers[i]) != VK_SUCCESS)
			throw std::runtime_error("failed to create framebuffer!");
	}
}

void Swapchain::createDepthResources(Device* deviceInfo)
{
	VkFormat depthFormat = findDepthFormat(deviceInfo);
	
	depthImage = new Image(deviceInfo, swapchainExtent.width, swapchainExtent.height, 1, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_DEPTH_BIT);
}

VkFormat Swapchain::findDepthFormat(Device* deviceInfo)
{
	return deviceInfo->findSupportedFormat({ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT }, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}