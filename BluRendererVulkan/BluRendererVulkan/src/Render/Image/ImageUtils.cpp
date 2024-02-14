#include "ImageUtils.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <stdexcept>
#include <string>

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
	newImage->transitionImageLayout(deviceInfo, commandPool, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, std::nullopt);
	newImage->copyImageFromBuffer(deviceInfo, commandPool, buffer, std::nullopt);
	//transitioned to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL while generating mipmaps

	buffer->freeBuffer(deviceInfo);
	delete buffer;
	newImage->generateMipmaps(deviceInfo, commandPool);
	newImage->createImageView(deviceInfo, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_2D);
	newImage->createTextureSampler(deviceInfo);

	return newImage;
}

Image* ImageUtils::createCubemapFromPath(Device* deviceInfo, CommandPool* commandPool, const std::string path, const std::string fileExt)
{
	const int CUBEMAP_FACES = 6;

	const std::vector<const char*> cubemapFilePath{
		(path + "_right" + fileExt).c_str(),
		(path + "_left" + fileExt).c_str(),
		(path + "_top" + fileExt).c_str(),
		(path + "_bottom" + fileExt).c_str(),
		(path + "_back" + fileExt).c_str(),
		(path + "_front" + fileExt).c_str()};


	Buffer* stagingBuffer = new Buffer(deviceInfo, 2048 * 2048 * 6 * 4, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	std::vector<Image*> images;
	size_t offset = 0;
	uint32_t mipLevels, width, height;
	// Setup buffer copy regions for each face including all of its miplevels
	std::vector<VkBufferImageCopy> bufferCopyRegions;

	for (size_t face = 0; face < static_cast<int>(CUBEMAP_FACES); face++)
	{
		int texWidth, texHeight, texChannels;
		stbi_uc* pixels = stbi_load(cubemapFilePath[face], &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
		VkDeviceSize imageSize = texWidth * texHeight * 4;
		mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;
		width = texWidth;
		height = texHeight;
		
		if (!pixels) {
			throw std::runtime_error("failed to load texture image!");
		}

		stagingBuffer->copyData(deviceInfo, pixels, offset, imageSize, 0);

		stbi_image_free(pixels);

		for (uint32_t level = 0; level < mipLevels; level++)
		{
			VkBufferImageCopy bufferCopyRegion = {};
			bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			bufferCopyRegion.imageSubresource.mipLevel = level;
			bufferCopyRegion.imageSubresource.baseArrayLayer = face;
			bufferCopyRegion.imageSubresource.layerCount = 1;
			bufferCopyRegion.imageExtent.width = texWidth >> level;
			bufferCopyRegion.imageExtent.height = texHeight >> level;
			bufferCopyRegion.imageExtent.depth = 1;
			bufferCopyRegion.bufferOffset = offset;
			bufferCopyRegions.push_back(bufferCopyRegion);
		}
		offset += imageSize;
	}

	// Image barrier for optimal image (target)
		// Set initial layout for all array layers (faces) of the optimal (target) tiled texture
	VkImageSubresourceRange subresourceRange = {};
	subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subresourceRange.baseMipLevel = 0;
	subresourceRange.levelCount = mipLevels;
	subresourceRange.layerCount = 6;

	Image* cubemap = new Image(deviceInfo, width * CUBEMAP_FACES, height * CUBEMAP_FACES, mipLevels, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	cubemap->transitionImageLayout(deviceInfo, commandPool, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subresourceRange);
	cubemap->copyImageFromBuffer(deviceInfo, commandPool, stagingBuffer, bufferCopyRegions);
	cubemap->transitionImageLayout(deviceInfo, commandPool, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, subresourceRange);
	cubemap->createTextureSampler(deviceInfo);
	cubemap->createImageView(deviceInfo, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_CUBE, subresourceRange);

	stagingBuffer->freeBuffer(deviceInfo);
	delete stagingBuffer;

	return cubemap;
	
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