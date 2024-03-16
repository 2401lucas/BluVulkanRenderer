#pragma once
#include <glm/glm.hpp>

#include "../Buffer/Buffer.h"
#include "../Command/CommandPool.h"
#include "../Device/Device.h"

class Image {
 public:
  Image(Device* deviceInfo, uint32_t width, uint32_t height, uint32_t mipLevels,
        VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling,
        VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
        VkImageAspectFlags aspectFlags);
  Image(Device* deviceInfo, uint32_t width, uint32_t height, uint32_t mipLevels,
        VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
        VkMemoryPropertyFlags properties, VkImageAspectFlags aspectFlags);
  Image(Device* deviceInfo, uint32_t width, uint32_t height, uint32_t mipLevels,
        VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling,
        VkImageUsageFlags usage, VkMemoryPropertyFlags properties);
  void cleanup(Device* deviceInfo);

  VkImage getImage();
  VkDeviceMemory getImageMemory();
  VkImageView getImageView();
  VkSampler getImageSampler();
  void createImageView(
      Device* deviceInfo, VkImageAspectFlags flags,
      VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D,
      std::optional<VkImageSubresourceRange> subresourceRange = std::nullopt);
  void createTextureSampler(Device* deviceInfo);
  void transitionImageLayout(
      Device* deviceInfo, CommandPool* commandPool, VkImageLayout newLayout,
      std::optional<VkImageSubresourceRange> subresourceRange);
  void copyImageFromBuffer(
      Device* deviceInfo, CommandPool* commandPool, Buffer* srcBuffer,
      std::optional<std::vector<VkBufferImageCopy>> bufferCopyRegion);
  void generateMipmaps(Device* deviceInfo, CommandPool* commandPool);

 private:
  void createImage(Device*, uint32_t, uint32_t, VkSampleCountFlagBits, VkFormat,
                   VkImageTiling, VkImageUsageFlags, VkMemoryPropertyFlags);
  uint32_t findMemoryType(Device*, uint32_t, VkMemoryPropertyFlags);
  bool hasStencilComponent(VkFormat);

  uint32_t width;
  uint32_t height;
  // VkImage->Raw Image
  // VkImageView->How to interact with VkImage(2D, Mip levels, Format, Image
  // Aspect...) VkSampler->Reads data from Imageview, applies filtering and
  // other shader transfomations
  VkImage image;
  VkDeviceMemory imageMemory;
  VkImageView imageView;
  bool hasSampler = false;
  VkSampler imageSampler;
  VkFormat format;
  uint32_t mipLevels;
  VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;
  VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
};