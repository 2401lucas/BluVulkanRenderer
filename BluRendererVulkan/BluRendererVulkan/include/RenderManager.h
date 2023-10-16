#pragma once

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <vector>
#include <optional>
#include <unordered_map>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>
#include <array>
#include <string>

namespace Core {
	namespace Rendering {
		

	
		
		

		struct UniformBufferObject {
			alignas(16) glm::mat4 model;
			alignas(16) glm::mat4 view;
			alignas(16) glm::mat4 proj;
		};

		class RenderManager {
		public:
			RenderManager(GLFWwindow*);
			~RenderManager();
			
			void renderFrame();

		private:
			void createVkInstance();
			bool checkValidationLayerSupport();
			void selectPhysicalDevice();
			VkPhysicalDevice ChooseDevice(std::vector<VkPhysicalDevice>);
			int rateDeviceSuitability(VkPhysicalDevice);
			bool isDeviceSuitable(VkPhysicalDevice);
			bool checkDeviceExtensionSupport(VkPhysicalDevice);
			SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice);
			Core::Rendering::QueueFamilyIndices findQueueFamilies(VkPhysicalDevice);
			void createLogicalDevice();
			void createSwapChain(GLFWwindow* window);
			void cleanupSwapChain();
			void recreateSwapChain();
			VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>&);
			VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>&);
			VkExtent2D chooseSwapExtent(GLFWwindow* window, const VkSurfaceCapabilitiesKHR&);
			void createImageViews();
			void createRenderPass();
			void createDescriptorSetLayout();
			void createGraphicsPipeline();
			VkShaderModule createShaderModule(const std::vector<char>&);
			void createCommandPool();
			void createFramebuffers();
			void createDepthResources();
			VkFormat findDepthFormat();
			bool hasStencilComponent(VkFormat);
			VkFormat findSupportedFormat(const std::vector<VkFormat>&, VkImageTiling, VkFormatFeatureFlags);
			void createTextureImage();
			void generateMipmaps(VkImage, VkFormat, int32_t, int32_t, uint32_t);
			void createTextureImageView();
			VkImageView createImageView(VkImage, VkFormat, VkImageAspectFlags, uint32_t);
			void createTextureSampler();
			void createImage(uint32_t, uint32_t, uint32_t, VkSampleCountFlagBits, VkFormat, VkImageTiling, VkImageUsageFlags, VkMemoryPropertyFlags, VkImage&, VkDeviceMemory&);
			void loadModel();
			void createVertexBuffer();
			void createIndexBuffer();
			void copyBuffer(VkBuffer, VkBuffer, VkDeviceSize);
			VkCommandBuffer beginSingleTimeCommands();
			void endSingleTimeCommands(VkCommandBuffer);
			void transitionImageLayout(VkImage, VkFormat, VkImageLayout, VkImageLayout, uint32_t);
			void copyBufferToImage(VkBuffer, VkImage, uint32_t, uint32_t);
			void createBuffer(VkDeviceSize, VkBufferUsageFlags, VkMemoryPropertyFlags, VkBuffer&, VkDeviceMemory&);
			void createUniformBuffers();
			void createDescriptorPool();
			void createDescriptorSets();
			uint32_t findMemoryType(uint32_t, VkMemoryPropertyFlags);
			void createCommandBuffers();
			void createSyncObjects();
			void recordCommandBuffer(VkCommandBuffer, uint32_t);
			void updateUniformBuffer(uint32_t);
			VkSampleCountFlagBits getMaxUsableSampleCount();
			void createColorResources();
			
			GLFWwindow* window;
			uint32_t currentFrame = 0;
			VkInstance vkInstance;
			VkSurfaceKHR surface;
			VkPhysicalDevice physicalDevice;
			VkDevice logicalDevice;
			VkQueue graphicsQueue;
			VkQueue presentQueue;
		
			VkRenderPass renderPass;
			VkDescriptorSetLayout descriptorSetLayout;
			VkPipelineLayout pipelineLayout;
			VkPipeline graphicsPipeline;
			std::vector<VkFramebuffer> swapChainFramebuffers;
			VkCommandPool commandPool;
			uint32_t mipLevels;
			
			VkImage textureImage;
			VkDeviceMemory textureImageMemory;
			VkImageView textureImageView;
			VkSampler textureSampler;

			VkImage depthImage;
			VkDeviceMemory depthImageMemory;
			VkImageView depthImageView;

			VkBuffer vertexBuffer;
			VkDeviceMemory vertexBufferMemory;
			VkBuffer indexBuffer;
			VkDeviceMemory indexBufferMemory;
			VkDescriptorPool descriptorPool;
			std::vector<VkDescriptorSet> descriptorSets;
			std::vector<VkBuffer> uniformBuffers;
			std::vector<VkDeviceMemory> uniformBuffersMemory;
			std::vector<void*> uniformBuffersMapped;
			std::vector<VkCommandBuffer> commandBuffers;
			std::vector<VkSemaphore> imageAvailableSemaphores;
			std::vector<VkSemaphore> renderFinishedSemaphores;
			std::vector<VkFence> inFlightFences;
			VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;
			VkImage colorImage;
			VkDeviceMemory colorImageMemory;
			VkImageView colorImageView;

			bool framebufferResized = false;
		};
	}
}