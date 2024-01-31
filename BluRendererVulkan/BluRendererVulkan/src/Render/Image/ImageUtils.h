#pragma once
#include "Image.h"
#include "../src/Engine/Scene/Scene.h"

class ImageUtils {
public:
	ImageUtils() = delete;

	static std::vector<Image*> createTexturesFromCreateInfo(Device* deviceInfo, CommandPool* commandPool, std::vector<TextureInfo> materialInfo);
	static Image* createImageFromPath(Device* deviceInfo, CommandPool* commandPool, const char* path);
	static Image* createCubemapFromPath(Device* deviceInfo, CommandPool* commandPool, const std::string path, const std::string fileExt);
	static VkImageView createImageView(Device* deviceInfo, VkImage image, VkFormat imageFormat, VkImageAspectFlags aspectFlags, uint32_t mipLevels);
};