#pragma once
#include "Image.h"

class ImageUtils {
public:
	ImageUtils() = delete;

	static Image* createImageFromPath(Device* deviceInfo, CommandPool* commandPool, const char* path);
	static VkImageView createImageView(Device* deviceInfo, VkImage image, VkFormat imageFormat, VkImageAspectFlags aspectFlags, uint32_t mipLevels);
};