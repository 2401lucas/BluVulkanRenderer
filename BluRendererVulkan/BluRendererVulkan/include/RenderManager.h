#pragma once

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <vector>
#include <optional>
#include <glm/glm.hpp>
#include <array>

namespace Core {
	namespace Rendering {
		struct QueueFamilyIndices {
		public:
			std::optional<uint32_t> graphicsFamily;
			std::optional<uint32_t> presentFamily;
			
			bool isComplete() {
				return graphicsFamily.has_value() && presentFamily.has_value();
			}
		};

		struct SwapChainSupportDetails {
			VkSurfaceCapabilitiesKHR capabilities;
			std::vector<VkSurfaceFormatKHR> formats;
			std::vector<VkPresentModeKHR> presentModes;
		};

		struct Vertex {
			glm::vec2 pos;
			glm::vec3 color;
		
			static VkVertexInputBindingDescription getBindingDescription() {
				VkVertexInputBindingDescription bindingDescription{};
				bindingDescription.binding = 0;
				bindingDescription.stride = sizeof(Vertex);
				bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
				return bindingDescription;
			}

			static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
				std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};
				attributeDescriptions[0].binding = 0;
				attributeDescriptions[0].location = 0;
				attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
				attributeDescriptions[0].offset = offsetof(Vertex, pos);
				attributeDescriptions[1].binding = 0;
				attributeDescriptions[1].location = 1;
				attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
				attributeDescriptions[1].offset = offsetof(Vertex, color);

				return attributeDescriptions;
			}
		};

		struct UniformBufferObject {
			glm::mat4 model;
			glm::mat4 view;
			glm::mat4 proj;
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
			void createFramebuffers();
			void createCommandPool();
			void createVertexBuffer();
			void createIndexBuffer();
			void copyBuffer(VkBuffer, VkBuffer, VkDeviceSize);
			void createBuffer(VkDeviceSize, VkBufferUsageFlags, VkMemoryPropertyFlags, VkBuffer&, VkDeviceMemory&);
			void createUniformBuffers();
			void createDescriptorPool();
			void createDescriptorSets();
			uint32_t findMemoryType(uint32_t, VkMemoryPropertyFlags);
			void createCommandBuffers();
			void createSyncObjects();
			void recordCommandBuffer(VkCommandBuffer, uint32_t);
			void updateUniformBuffer(uint32_t);

			GLFWwindow* window;
			uint32_t currentFrame = 0;
			VkInstance vkInstance;
			VkSurfaceKHR surface;
			VkPhysicalDevice physicalDevice;
			VkDevice logicalDevice;
			VkQueue graphicsQueue;
			VkQueue presentQueue;
			VkSwapchainKHR swapChain;
			std::vector<VkImage> swapChainImages;
			VkFormat swapChainImageFormat;
			VkExtent2D swapChainExtent;
			std::vector<VkImageView> swapChainImageViews;
			VkRenderPass renderPass;
			VkDescriptorSetLayout descriptorSetLayout;
			VkPipelineLayout pipelineLayout;
			VkPipeline graphicsPipeline;
			std::vector<VkFramebuffer> swapChainFramebuffers;
			VkCommandPool commandPool;
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

			bool framebufferResized = false;

			const std::vector<Vertex> vertices = {
			{{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
			{{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
			{{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
			{{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}} };

			const std::vector<uint16_t> indices = { 0, 1, 2, 2, 3, 0 };
		};
	}
}