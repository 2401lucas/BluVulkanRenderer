#include "Image.h"

blu::core::Image* blu::core::Image::CreateImage(
    const VkDevice& device, const VmaAllocator& allocator, VkFormat format,
    uint32_t width, uint32_t height, uint32_t mip_levels,
    VkSampleCountFlagBits samples, VkImageTiling tiling,
    VkImageUsageFlags usage, VkMemoryPropertyFlags required_flags,
    VmaAllocationCreateFlags flags) {
  Image* new_image = new Image();
  VkImageCreateInfo image_create_info{
      .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      .imageType = VK_IMAGE_TYPE_2D,
      .format = format,
      .extent{
          .width = width,
          .height = height,
          .depth = 1,
      },
      .mipLevels = mip_levels,
      .arrayLayers = 1,
      .samples = samples,
      .tiling = tiling,
      .usage = usage,
  };

  VmaAllocationCreateInfo vma_create_info{
      .flags = flags,
      .requiredFlags = required_flags,
  };

  VmaAllocationInfo alloc_info;
  vmaCreateImage(allocator, &image_create_info, &vma_create_info,
                 &new_image->image, &new_image->alloc, &alloc_info);

  return new_image;
}