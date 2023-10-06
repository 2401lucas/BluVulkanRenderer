//Platform specific headers
#define VK_USE_PLATFORM_WIN32_KHR

#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#define GLFW_INCLUDE_VULKAN
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include "../include/RenderManager.h"
#include "../include/FileManager.h"

const int MAX_FRAMES_IN_FLIGHT = 2;

// TODO: Update Platform Dependancies 
const char* instanceExtensions[] = { VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME, };
const char* deviceExtensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME, };
const char* validationLayers[] = { "VK_LAYER_KHRONOS_validation" };

#ifdef NDEBUG
	const bool enableValidationLayers = false;
#else
	const bool enableValidationLayers = true;
#endif

const VkApplicationInfo appInfo = {
	.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
	.pNext = NULL,
	.pApplicationName = "OBJ Renderer",
	.applicationVersion = 0,
	.pEngineName = "BluRendererVulkan",
	.engineVersion = 0,
	.apiVersion = VK_API_VERSION_1_3,
};

Core::Rendering::RenderManager::RenderManager(GLFWwindow* window) {
	this->window = window;
	createVkInstance();
	// TODO: SETUP Validation Debug Messenger
	if(glfwCreateWindowSurface(vkInstance, window, nullptr, &surface) != VK_SUCCESS)
		throw std::runtime_error("failed to create window surface!");
	selectPhysicalDevice();
	createLogicalDevice();
	createSwapChain(window);
	createImageViews();
	createRenderPass();
	createGraphicsPipeline();
	createFramebuffers();
	createCommandPool();
	createVertexBuffer();
	createIndexBuffer();
	createCommandBuffers();
	createSyncObjects();
}

Core::Rendering::RenderManager::~RenderManager() {
	vkDeviceWaitIdle(logicalDevice);

	cleanupSwapChain();

	vkDestroyBuffer(logicalDevice, vertexBuffer, nullptr);
	vkFreeMemory(logicalDevice, vertexBufferMemory, nullptr);
	vkDestroyBuffer(logicalDevice, indexBuffer, nullptr);
	vkFreeMemory(logicalDevice, indexBufferMemory, nullptr);
	
	vkDestroyPipeline(logicalDevice, graphicsPipeline, nullptr);
	vkDestroyPipelineLayout(logicalDevice, pipelineLayout, nullptr);
	vkDestroyRenderPass(logicalDevice, renderPass, nullptr);

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		vkDestroySemaphore(logicalDevice, renderFinishedSemaphores[i], nullptr);
		vkDestroySemaphore(logicalDevice, imageAvailableSemaphores[i], nullptr);
		vkDestroyFence(logicalDevice, inFlightFences[i], nullptr);
	}

	vkDestroyCommandPool(logicalDevice, commandPool, nullptr);
	vkDestroyDevice(logicalDevice, nullptr);
	vkDestroySurfaceKHR(vkInstance, surface, nullptr);
	vkDestroyInstance(vkInstance, nullptr);
}

void Core::Rendering::RenderManager::createVkInstance()
{
	if (enableValidationLayers && !checkValidationLayerSupport())
		throw std::runtime_error("validation layers requested, but not available!");

	// TODO: Check that extensions match the extensions required
	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
	std::vector<VkExtensionProperties> extensions(extensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
	
	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;
	createInfo.enabledExtensionCount = sizeof(instanceExtensions) / sizeof(instanceExtensions[0]);
	createInfo.ppEnabledExtensionNames = instanceExtensions;
	//DEBUG BUILD ONLY ---------
	createInfo.enabledLayerCount = sizeof(validationLayers) / sizeof(validationLayers[0]);
	createInfo.ppEnabledLayerNames = validationLayers;


	if (vkCreateInstance(&createInfo, nullptr, &vkInstance) != VK_SUCCESS) {
		throw std::runtime_error("Vulkan instance creation failed!");
	}
}

bool Core::Rendering::RenderManager::checkValidationLayerSupport()
{
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
	
	for (const char* layerName : validationLayers) {
		bool layerFound = false;
		for (const auto& layerProperties : availableLayers) {
			if (strcmp(layerName, layerProperties.layerName) == 0) {
				layerFound = true;
				break;
			}
		}
		if (!layerFound)
			return false;
	}

	return true;
}

void Core::Rendering::RenderManager::selectPhysicalDevice()
{
	// Select a supported render device
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(vkInstance, &deviceCount, nullptr);

	if (deviceCount == 0) {
		throw std::runtime_error("No physical devices found!");
	}

	std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
	vkEnumeratePhysicalDevices(vkInstance, &deviceCount, physicalDevices.data());
	physicalDevice = ChooseDevice(physicalDevices);
}

VkPhysicalDevice Core::Rendering::RenderManager::ChooseDevice(std::vector<VkPhysicalDevice> deviceList)
{
	std::multimap<int, VkPhysicalDevice> candidates;

	for (auto& device : deviceList)
	{
		int score = rateDeviceSuitability(device);
		candidates.insert(std::make_pair(score, device));
	}

	if (candidates.rbegin()->first > 0) {
		return candidates.rbegin()->second;
	}
	else {
		throw std::runtime_error("failed to find a suitable GPU!");
	}

}

int Core::Rendering::RenderManager::rateDeviceSuitability(VkPhysicalDevice device)
{
	VkPhysicalDeviceProperties deviceProperties;
	VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceProperties(device, &deviceProperties);
	vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

	// Required Features
	if (!deviceFeatures.geometryShader) {
		return 0;
	}

	int score = 0;

	if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
		score += 10000;
	}
	// Maximum possible size of textures affects graphics quality
	score += deviceProperties.limits.maxImageDimension2D;

	return score;
}

bool Core::Rendering::RenderManager::isDeviceSuitable(VkPhysicalDevice device)
{
	QueueFamilyIndices indices = findQueueFamilies(device);

	bool extensionsSupported = checkDeviceExtensionSupport(device);

	bool swapChainAdequate = false;
	if (extensionsSupported)
	{
		SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
		swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
	}

	return indices.isComplete() && extensionsSupported && swapChainAdequate;
}

bool Core::Rendering::RenderManager::checkDeviceExtensionSupport(VkPhysicalDevice device)
{
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());
	
	for (const char* requiredExtension : deviceExtensions) {
		bool extensionFound = false;
		for (const auto& availibleExtension : availableExtensions) {
			if (strcmp(requiredExtension, availibleExtension.extensionName) == 0) {
				extensionFound = true;
				break;
			}
		}
		if (!extensionFound)
			return false;
	}

	return true;
}

Core::Rendering::SwapChainSupportDetails Core::Rendering::RenderManager::querySwapChainSupport(VkPhysicalDevice device)
{
	SwapChainSupportDetails details;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
	if (formatCount != 0) {
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
	}

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
	if (presentModeCount != 0) {
		details.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
	}

	return details;
}

Core::Rendering::QueueFamilyIndices Core::Rendering::RenderManager::findQueueFamilies(VkPhysicalDevice device)
{
	QueueFamilyIndices indices;
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

	for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilies.size()); ++i) {
		const VkQueueFamilyProperties& queueFamily = queueFamilies[i];

		if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			indices.graphicsFamily = i;
		}

		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
		if (presentSupport) {
			indices.presentFamily = i;
		}

		if (indices.isComplete()) {
			break;
		}
	}

	return indices;
}

void Core::Rendering::RenderManager::createLogicalDevice()
{
	QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };
	
	float queuePriority = 1.0f;
	for (uint32_t queueFamily : uniqueQueueFamilies) {
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkPhysicalDeviceFeatures deviceFeatures = { };

	VkDeviceCreateInfo deviceCreateInfo = {};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
	deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
	deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions;
	deviceCreateInfo.enabledExtensionCount = sizeof(deviceExtensions) / sizeof(deviceExtensions[0]);

	if (enableValidationLayers) {
		deviceCreateInfo.ppEnabledLayerNames = validationLayers;
		deviceCreateInfo.enabledLayerCount = sizeof(validationLayers) / sizeof(validationLayers[0]);
	}
	else {
		deviceCreateInfo.enabledLayerCount = 0;
	}

	if (vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &logicalDevice) != VK_SUCCESS) {
		throw std::runtime_error("failed to create logical device!");
	}

	vkGetDeviceQueue(logicalDevice, indices.graphicsFamily.value(), 0, &graphicsQueue); 
	vkGetDeviceQueue(logicalDevice, indices.presentFamily.value(), 0, &presentQueue);
}

//TODO: HANDLE WINDOW RESIZE
void Core::Rendering::RenderManager::createSwapChain(GLFWwindow* window)
{
	SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);
	VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
	VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
	VkExtent2D extent = chooseSwapExtent(window, swapChainSupport.capabilities);

	uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
	if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
		imageCount = swapChainSupport.capabilities.maxImageCount;
	}
	VkSwapchainCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = surface;
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // TODO: VK_IMAGE_USAGE_TRANSFER_DST_BIT for post processing

	QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
	uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };
	if (indices.graphicsFamily != indices.presentFamily) {
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else { //TODO: REMOVE VK_SHARING_MODE_CONCURRENT option for performance and do ownership transfer
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0;
		createInfo.pQueueFamilyIndices = nullptr;
	}

	createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = VK_NULL_HANDLE;

	if (vkCreateSwapchainKHR(logicalDevice, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
		throw std::runtime_error("failed to create swap chain!");
	}

	vkGetSwapchainImagesKHR(logicalDevice, swapChain, &imageCount, nullptr);
	swapChainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(logicalDevice, swapChain, &imageCount, swapChainImages.data());
	swapChainImageFormat = surfaceFormat.format;
	swapChainExtent = extent;
}

void Core::Rendering::RenderManager::cleanupSwapChain()
{
	for (auto framebuffer : swapChainFramebuffers) {
		vkDestroyFramebuffer(logicalDevice, framebuffer, nullptr);
	}
	for (auto imageView : swapChainImageViews) {
		vkDestroyImageView(logicalDevice, imageView, nullptr);
	}
	vkDestroySwapchainKHR(logicalDevice, swapChain, nullptr);
}

void Core::Rendering::RenderManager::recreateSwapChain()
{
	vkDeviceWaitIdle(logicalDevice);

	cleanupSwapChain();

	createSwapChain(window);
	createImageViews();
	createFramebuffers();
}

VkSurfaceFormatKHR Core::Rendering::RenderManager::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
	for (const auto& availableFormat : availableFormats) {
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			return availableFormat;
		}
	}

	//TODO: Return rank and return best from available formats
	return availableFormats[0];
}

VkPresentModeKHR Core::Rendering::RenderManager::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
	// Prefer VK_PRESENT_MODE_MAILBOX_KHR on pc
	// TODO: Prefer VK_PRESENT_MODE_FIFO_KHR on mobile due to higher energy usage
	
	for (const auto& availablePresentMode : availablePresentModes) {
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
			return availablePresentMode;		
		}
	}
		
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Core::Rendering::RenderManager::chooseSwapExtent(GLFWwindow* window, const VkSurfaceCapabilitiesKHR& capabilities)
{
	// TODO: Investigate return capabilities.currentExtent;
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	VkExtent2D actualExtent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };

	//TODO: USE OWN MATH LIBRARY
	actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
	actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

	return actualExtent;
}

void Core::Rendering::RenderManager::createImageViews()
{
	swapChainImageViews.resize(swapChainImages.size());

	for (auto i = 0; i < swapChainImages.size(); i++) {
		VkImageViewCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = swapChainImages[i];
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = swapChainImageFormat;
		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;

		if (vkCreateImageView(logicalDevice, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to create image views!");
		}
	}
}

void Core::Rendering::RenderManager::createRenderPass()
{
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = swapChainImageFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	
	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	
	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &colorAttachment;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	if (vkCreateRenderPass(logicalDevice, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS)
		throw std::runtime_error("failed to create render pass!");

}

void Core::Rendering::RenderManager::createGraphicsPipeline()
{
	auto vertShaderCode = Core::System::FileManager::readBinary("shaders/vert.spv");
	auto fragShaderCode = Core::System::FileManager::readBinary("shaders/frag.spv");

	VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
	VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

	VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main";
	vertShaderStageInfo.pSpecializationInfo = nullptr;

	VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";
	fragShaderStageInfo.pSpecializationInfo = nullptr;

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	auto bindingDescription = Vertex::getBindingDescription();
	auto attributeDescription = Vertex::getAttributeDescriptions();

	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription; // Optional
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescription.size());
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescription.data();

	std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	dynamicState.pDynamicStates = dynamicStates.data();

	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = swapChainExtent;

	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	//viewportState.pViewports = &viewport; NON DYNAMIC
	viewportState.scissorCount = 1;
	//viewportState.pScissors = &scissor; NON DYNAMIC

	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f; // Optional
	rasterizer.depthBiasClamp = 0.0f; // Optional
	rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 1.0f; // Optional
	multisampling.pSampleMask = nullptr; // Optional
	multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
	multisampling.alphaToOneEnable = VK_FALSE; // Optional

	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f; // Optional
	colorBlending.blendConstants[1] = 0.0f; // Optional
	colorBlending.blendConstants[2] = 0.0f; // Optional
	colorBlending.blendConstants[3] = 0.0f; // Optional

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 0; // Optional
	pipelineLayoutInfo.pSetLayouts = nullptr; // Optional
	pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
	pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional
	
	if (vkCreatePipelineLayout(logicalDevice, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
		throw std::runtime_error("failed to create pipeline layout!");

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = nullptr; // Optional
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = &dynamicState;
	pipelineInfo.layout = pipelineLayout;
	pipelineInfo.renderPass = renderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
	pipelineInfo.basePipelineIndex = -1; // Optional

	if (vkCreateGraphicsPipelines(logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS)
		throw std::runtime_error("failed to create graphics pipeline!");

	vkDestroyShaderModule(logicalDevice, vertShaderModule, nullptr);
	vkDestroyShaderModule(logicalDevice, fragShaderModule, nullptr);
}

VkShaderModule Core::Rendering::RenderManager::createShaderModule(const std::vector<char>& code) {
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(logicalDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
		throw std::runtime_error("failed to create shader module!");
	}
	return shaderModule;
}

void Core::Rendering::RenderManager::createFramebuffers()
{
	swapChainFramebuffers.resize(swapChainImageViews.size());
	for (size_t i = 0; i < swapChainImageViews.size(); i++) {
		VkImageView attachments[] = { swapChainImageViews[i] };
		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = renderPass;
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.pAttachments = attachments;
		framebufferInfo.width = swapChainExtent.width;
		framebufferInfo.height = swapChainExtent.height;
		framebufferInfo.layers = 1;
		if (vkCreateFramebuffer(logicalDevice, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS)
			throw std::runtime_error("failed to create framebuffer!");
	}
}

void Core::Rendering::RenderManager::createCommandPool()
{
	QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

	if (vkCreateCommandPool(logicalDevice, &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
		throw std::runtime_error("failed to create command pool!");
}

// TODO: Change from VK_MEMORY_PROPERTY_HOST_COHERENT_BIT to explicite
//vkflushMappedMemoryRanges and vkInvalidateMappedMemoryRanges
void Core::Rendering::RenderManager::createVertexBuffer()
{
	VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(logicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, vertices.data(), (size_t)bufferSize);
	vkUnmapMemory(logicalDevice, stagingBufferMemory);

	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);

	copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

	vkDestroyBuffer(logicalDevice, stagingBuffer, nullptr);
	vkFreeMemory(logicalDevice, stagingBufferMemory, nullptr);
}

void Core::Rendering::RenderManager::createIndexBuffer()
{
	VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(logicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, indices.data(), (size_t)bufferSize);
	vkUnmapMemory(logicalDevice, stagingBufferMemory);

	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);

	copyBuffer(stagingBuffer, indexBuffer, bufferSize);

	vkDestroyBuffer(logicalDevice, stagingBuffer, nullptr);
	vkFreeMemory(logicalDevice, stagingBufferMemory, nullptr);
}

void Core::Rendering::RenderManager::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = commandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(logicalDevice, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	VkBufferCopy copyRegion{};
	copyRegion.srcOffset = 0;
	copyRegion.dstOffset = 0;
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	//TODO: USE FENCE
	vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(graphicsQueue);

	vkFreeCommandBuffers(logicalDevice, commandPool, 1, &commandBuffer);
}

void Core::Rendering::RenderManager::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
{
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(logicalDevice, &bufferInfo, nullptr, &vertexBuffer) != VK_SUCCESS)
		throw std::runtime_error("failed to create vertex buffer!");

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(logicalDevice, vertexBuffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	if (vkAllocateMemory(logicalDevice, &allocInfo, nullptr, &vertexBufferMemory) != VK_SUCCESS)
		throw std::runtime_error("failed to allocate vertex buffer memory!");

	vkBindBufferMemory(logicalDevice, vertexBuffer, vertexBufferMemory, 0);
}

uint32_t Core::Rendering::RenderManager::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
		if (typeFilter & (1 << i) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
			return i;

	throw std::runtime_error("failed to find suitable memory type!");
}

void Core::Rendering::RenderManager::createCommandBuffers()
{
	commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

	VkCommandBufferAllocateInfo allocInfo{ };
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

	if (vkAllocateCommandBuffers(logicalDevice, &allocInfo, commandBuffers.data()) != VK_SUCCESS)
		throw std::runtime_error("failed to create command buffer!");
}

void Core::Rendering::RenderManager::createSyncObjects()
{
	imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		if (vkCreateSemaphore(logicalDevice, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
			vkCreateSemaphore(logicalDevice, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
			vkCreateFence(logicalDevice, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to create semaphores!");
		}
	}
}

void Core::Rendering::RenderManager::recordCommandBuffer(VkCommandBuffer buffer, uint32_t imageIndex)
{
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = 0;
	beginInfo.pInheritanceInfo = nullptr;

	if (vkBeginCommandBuffer(buffer, &beginInfo) != VK_SUCCESS)
		throw std::runtime_error("failed to begin recording command buffer!");

	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = renderPass;
	renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
	renderPassInfo.renderArea.offset = { 0,0 };
	renderPassInfo.renderArea.extent = swapChainExtent;

	VkClearValue clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
	renderPassInfo.clearValueCount = 1;
	renderPassInfo.pClearValues = &clearColor;

	vkCmdBeginRenderPass(buffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
	
	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(swapChainExtent.width);
	viewport.height = static_cast<float>(swapChainExtent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(buffer, 0, 1, &viewport);
	
	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = swapChainExtent;
	vkCmdSetScissor(buffer, 0, 1, &scissor);

	VkBuffer vertexBuffers[] = { vertexBuffer };
	VkDeviceSize offsets[] = { 0 };
	
	vkCmdBindVertexBuffers(buffer, 0, 1, vertexBuffers, offsets);
	vkCmdBindIndexBuffer(buffer, indexBuffer, 0, VK_INDEX_TYPE_UINT16);

	vkCmdDraw(buffer, static_cast<uint32_t>(vertices.size()), 1, 0, 0);
	vkCmdDrawIndexed(buffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

	vkCmdEndRenderPass(buffer);

	if (vkEndCommandBuffer(buffer) != VK_SUCCESS)
		throw std::runtime_error("failed to record command buffer!");
}

void Core::Rendering::RenderManager::renderFrame()
{
	//Wait for previous frame
	vkWaitForFences(logicalDevice, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

	// Acquire a swap chain image
	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(logicalDevice, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);
	
	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		recreateSwapChain();
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
		throw std::runtime_error("failed to acquire swap chain image!");

	vkResetFences(logicalDevice, 1, &inFlightFences[currentFrame]);

	// Record rendering commands to command buffers
	vkResetCommandBuffer(commandBuffers[currentFrame], 0);
	recordCommandBuffer(commandBuffers[currentFrame], imageIndex);

	// Submit command buffers to the graphics queue
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame]};
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffers[currentFrame];
	
	VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame]};
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	result = vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]);
	if (result != VK_SUCCESS)
		throw std::runtime_error("failed to submit draw command buffer!");

	// Present the rendered image
	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;
	
	VkSwapchainKHR swapChains[] = { swapChain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = nullptr;	

	result = vkQueuePresentKHR(presentQueue, &presentInfo);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
		framebufferResized = false;
		recreateSwapChain();
	} else if (result != VK_SUCCESS)
		throw std::runtime_error("failed to present swap chain image!");

	currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}