#include <glm/glm.hpp>
#include <vulkan/vulkan_core.h>
#include <optional>

class Image {
public:

	Image(VkPhysicalDevice, VkDevice, uint32_t, uint32_t, uint32_t, VkSampleCountFlagBits, VkFormat, VkImageTiling, VkImageUsageFlags, VkMemoryPropertyFlags, VkImageAspectFlags);
	Image(VkPhysicalDevice, VkDevice, uint32_t, uint32_t, uint32_t, VkFormat, VkImageTiling, VkImageUsageFlags, VkMemoryPropertyFlags, VkImageAspectFlags);
	~Image();

private:
	void createImage(VkPhysicalDevice, VkDevice, uint32_t, uint32_t, VkSampleCountFlagBits, VkFormat, VkImageTiling, VkImageUsageFlags, VkMemoryPropertyFlags, VkImage&, VkDeviceMemory&);
	uint32_t findMemoryType(VkPhysicalDevice, uint32_t, VkMemoryPropertyFlags);
	void createImageView(VkDevice, VkFormat, VkImageAspectFlags);
	void createTextureSampler(VkPhysicalDevice, VkDevice);

	VkImage image;
	VkDeviceMemory imageMemory;
	VkImageView imageView;
	std::optional<VkSampler> imageSampler;
	uint32_t mipLevels;
	VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;
};