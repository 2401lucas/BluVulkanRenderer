#ifndef IMAGE_H
#define IMAGE_H

#include <Vulkan/vulkan.h>
#include <vk_mem_alloc.h>

#include "Buffer.h"

namespace blu::core {
class Image {
 public:
  // Required
  VkImage image = VK_NULL_HANDLE;
  VkImageView view = VK_NULL_HANDLE;
  VkSampler sampler = VK_NULL_HANDLE;
  VmaAllocation alloc = VK_NULL_HANDLE;
  VkDeviceSize size;
  VkDeviceSize offset = 0;
  // Optional
  VkMemoryRequirements memReqs;
  void* mappedData = nullptr;

  void Destroy(const VkDevice& device, const VmaAllocator& allocator);

  static blu::core::Image* CreateImage(
      const VkDevice& device, const VmaAllocator& allocator, VkFormat format,
      uint32_t width, uint32_t height, uint32_t mip_levels,
      VkSampleCountFlagBits samples, VkImageTiling tiling,
      VkImageUsageFlags usage, VkMemoryPropertyFlags required_flags,
      VmaAllocationCreateFlags flags = 0);
  static blu::core::Image* CreateImage(
      const VkDevice& device, const VmaAllocator& allocator, VkCommandBuffer,
      uint32_t src_queue, uint32_t dst_queue, VkFormat format, uint32_t width,
      uint32_t height, uint32_t mip_levels, VkSampleCountFlagBits samples,
      VkImageTiling tiling, VkImageUsageFlags usage,
      VkMemoryPropertyFlags required_flags, unsigned char* data,
      blu::core::Buffer*& stg_buffer, VmaAllocationCreateFlags flags = 0);

  static void CreateImageView(
      const VkDevice& device, blu::core::Image* image, VkFormat format,
      const VkImageSubresourceRange& subresource_range,
      VkImageViewType image_view_type = VK_IMAGE_VIEW_TYPE_2D);

  static void CreateImageSampler(
      const VkDevice& device,
      const VkPhysicalDeviceProperties& physical_device_properties,
      blu::core::Image* image);

  static void ImageLayoutTransition(
      VkCommandBuffer, VkImage image, VkPipelineStageFlags src_stage_mask,
      VkPipelineStageFlags dst_stage_mask, VkAccessFlags src_access_mask,
      VkAccessFlags dst_access_mask, VkImageLayout old_layout,
      VkImageLayout new_layout,
      const VkImageSubresourceRange& subresource_range,
      uint32_t src_queue_index = VK_QUEUE_FAMILY_IGNORED,
      uint32_t dst_queue_index = VK_QUEUE_FAMILY_IGNORED);
  static void ImageLayoutTransition(
      VkCommandBuffer, VkImage image, VkImageLayout old_layout,
      VkImageLayout new_layout,
      const VkImageSubresourceRange& subresource_range,
      uint32_t src_queue_index = VK_QUEUE_FAMILY_IGNORED,
      uint32_t dst_queue_index = VK_QUEUE_FAMILY_IGNORED);

 private:
  static VkAccessFlags GetAccessFlags(VkImageLayout layout);
  static VkPipelineStageFlags GetPipelineStageFlags(VkImageLayout layout);
};
}  // namespace blu::core
#endif
