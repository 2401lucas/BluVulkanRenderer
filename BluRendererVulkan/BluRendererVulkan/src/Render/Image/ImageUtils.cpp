#include "ImageUtils.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <stdexcept>

std::vector<Image*> ImageUtils::createTexturesFromCreateInfo(Device* deviceInfo, CommandPool* commandPool, std::vector<TextureInfo> materialInfo)
{
	std::vector<Image*> newTextures;

	for (auto& newT : materialInfo) {
		newTextures.push_back(createImageFromPath(deviceInfo, commandPool, newT.fileName.c_str()));
	}

	return newTextures;
}

Image* ImageUtils::createImageFromPath(Device* deviceInfo, CommandPool* commandPool, const char* path)
{
	int texWidth, texHeight, texChannels;
	stbi_uc* pixels = stbi_load(path, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
	VkDeviceSize imageSize = texWidth * texHeight * 4;
	uint32_t mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

	if (!pixels) {
		throw std::runtime_error("failed to load texture image!");
	}

	Buffer* buffer = new Buffer(deviceInfo, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	buffer->copyData(deviceInfo, pixels, 0, imageSize, 0);

	stbi_image_free(pixels);
	
	Image* newImage = new Image(deviceInfo, texWidth, texHeight, mipLevels, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	newImage->transitionImageLayout(deviceInfo, commandPool, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	newImage->copyImageFromBuffer(deviceInfo, commandPool, buffer);
	//transitioned to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL while generating mipmaps

	buffer->freeBuffer(deviceInfo);
	delete buffer;
	newImage->generateMipmaps(deviceInfo, commandPool);
	newImage->createImageView(deviceInfo, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_2D);
	newImage->createTextureSampler(deviceInfo);

	return newImage;
}

VkImageView ImageUtils::createImageView(Device* deviceInfo, VkImage image, VkFormat imageFormat, VkImageAspectFlags aspectFlags, uint32_t mipLevels)
{
	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = imageFormat;
	viewInfo.subresourceRange.aspectMask = aspectFlags;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = mipLevels;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	VkImageView imageView;
	if (vkCreateImageView(deviceInfo->getLogicalDevice(), &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
		throw std::runtime_error("failed to create texture image view!");
	}

	return imageView;
}