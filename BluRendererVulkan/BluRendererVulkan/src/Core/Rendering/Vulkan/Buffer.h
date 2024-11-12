#ifndef BUFFER_H
#define BUFFER_H

#include <Vulkan/vulkan.h>
#include <vk_mem_alloc.h>

namespace blu::core {
class Buffer {
 public:
  VkBuffer buffer;
  VmaAllocation alloc;
  VkDeviceSize size;
  VkDeviceSize offset = 0;
  void* mapped_data = nullptr;
  VkDeviceAddress device_address;

  void Destroy(const VmaAllocator& allocator);

  static blu::core::Buffer* CreateBuffer(const VkDevice& device,
                                         const VmaAllocator& allocator,
                                         VkDeviceSize size,
                                         VkBufferUsageFlags usage,
                                         VkMemoryPropertyFlags required_flags,
                                         VmaAllocationCreateFlags flags = 0);
};
}  // namespace blu::core
#endif