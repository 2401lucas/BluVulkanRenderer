#pragma once
#include <glm/glm.hpp>
#include "../src/Device/Device.h"
#include "../src/Command/CommandPool.h"
#include "../Buffer/Buffer.h"

class Image {
public:
	Image(Device* deviceInfo, uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImageAspectFlags aspectFlags);
	Image(Device* deviceInfo, uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImageAspectFlags aspectFlags);
	Image(Device* deviceInfo, uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties);
	void cleanup(Device* deviceInfo);

	VkImage getImage();
	VkDeviceMemory getImageMemory();
	VkImageView getImageView();
	VkSampler getImageSampler();
	void createImageView(Device* deviceInfo, VkImageAspectFlags flags);
	void createTextureSampler(Device* deviceInfo);
	void transitionImageLayout(Device* deviceInfo, CommandPool* commandPool, VkImageLayout newLayout);
	void copyImageFromBuffer(Device* deviceInfo, CommandPool* commandPool, Buffer* srcBuffer);
	void generateMipmaps(Device* deviceInfo, CommandPool *commandPool);

	static VkImageView createImageView(Device* deviceInfo , VkImage image, VkFormat imageFormat, VkImageAspectFlags aspectFlags, uint32_t mipLevels);
private:
	void createImage(Device*, uint32_t, uint32_t, VkSampleCountFlagBits, VkFormat, VkImageTiling, VkImageUsageFlags, VkMemoryPropertyFlags);
	uint32_t findMemoryType(Device*, uint32_t, VkMemoryPropertyFlags);
	bool hasStencilComponent(VkFormat);

	uint32_t width;
	uint32_t height;
	VkImage image;
	VkDeviceMemory imageMemory;
	VkImageView imageView;
	VkSampler imageSampler;
	VkFormat format;
	uint32_t mipLevels;
	VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;
	VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
};