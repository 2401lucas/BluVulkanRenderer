#ifndef IMAGE_H
#define IMAGE_H

#include <Vulkan/vulkan.h>
#include <vk_mem_alloc.h>

namespace blu::core {
class Image {
 public:
  // Required
  VkImage image = VK_NULL_HANDLE;
  VkImageView view = VK_NULL_HANDLE;
  VkSampler sampler = VK_NULL_HANDLE;
  VkImageLayout imageLayout;
  VmaAllocation alloc = VK_NULL_HANDLE;
  VkDeviceSize size;
  VkDeviceSize offset = 0;
  VkImageSubresourceRange subresourceRange;
  // Optional
  VkMemoryRequirements memReqs;
  void* mappedData = nullptr;

  static blu::core::Image* CreateImage(
      const VkDevice& device, const VmaAllocator& allocator, VkFormat format,
      uint32_t width, uint32_t height, uint32_t mipLevels,
      VkSampleCountFlagBits samples, VkImageTiling tiling,
      VkImageUsageFlags usage, VkMemoryPropertyFlags required_flags,
      VmaAllocationCreateFlags flags = 0);
};
}  // namespace blu::core
#endif
