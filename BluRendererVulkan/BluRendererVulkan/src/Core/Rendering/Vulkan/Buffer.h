#ifndef BUFFER_H
#define BUFFER_H

#include <Vulkan/vulkan.h>
#include <vk_mem_alloc.h>

struct BufferInfo {
  VkDeviceAddress address;
  VkDeviceSize offset;
  VkDeviceSize size;
};

namespace blu::core {
class Buffer {
 public:
  VkBuffer buffer;
  VmaAllocation alloc;
  VkDeviceSize size;
  VkDeviceSize offset = 0;
  char* mapped_data = nullptr;
  VkDeviceAddress device_address;

  void Destroy(const VmaAllocator& allocator);

  BufferInfo GetBufferInfo();

  static blu::core::Buffer* CreateBuffer(const VkDevice& device,
                                         const VmaAllocator& allocator,
                                         VkDeviceSize size,
                                         VkBufferUsageFlags usage,
                                         VkMemoryPropertyFlags required_flags,
                                         VmaAllocationCreateFlags flags = 0);

  static void BufferMemoryBarrier(
      VkCommandBuffer command_buffer, VkBuffer buffer,
      VkPipelineStageFlags2 src_stage_mask,
      VkPipelineStageFlags2 dst_stage_mask, VkAccessFlagBits2 src_access_mask,
      VkAccessFlagBits2 dst_access_mask,
      uint32_t src_queue_index = VK_QUEUE_FAMILY_IGNORED,
      uint32_t dst_queue_index = VK_QUEUE_FAMILY_IGNORED);

  static void UploadToBuffer(const VkDevice& device,
                             const VmaAllocator& allocator,
                             blu::core::Buffer* dst_buffer,
                             VkDeviceSize dst_offset,
                             VkCommandBuffer copy_command, void* data,
                             VkDeviceSize size, VkDeviceSize src_offset,
                             blu::core::Buffer*& stg_buffer);
};
}  // namespace blu::core
#endif