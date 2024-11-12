#include "Buffer.h"

void blu::core::Buffer::Destroy(const VmaAllocator& allocator) {
  vmaDestroyBuffer(allocator, buffer, alloc);
}

blu::core::Buffer* blu::core::Buffer::CreateBuffer(
    const VkDevice& device, const VmaAllocator& allocator, VkDeviceSize size,
    VkBufferUsageFlags usage, VkMemoryPropertyFlags required_flags,
    VmaAllocationCreateFlags flags) {
  blu::core::Buffer* new_buffer = new blu::core::Buffer();
  VkBufferCreateInfo buf_ci{
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .size = size,
      .usage = usage,
  };

  VmaAllocationCreateInfo alloc_ci{
      .flags = flags,
      .requiredFlags = required_flags,
  };

  VmaAllocationInfo alloc_info;
  vmaCreateBuffer(allocator, &buf_ci, &alloc_ci, &new_buffer->buffer,
                  &new_buffer->alloc, &alloc_info);

  new_buffer->size = buf_ci.size;
  new_buffer->mapped_data = alloc_info.pMappedData;

  if (usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) {
    VkBufferDeviceAddressInfo info{
        .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        .buffer = new_buffer->buffer};

    new_buffer->device_address = vkGetBufferDeviceAddress(device, &info);
  }

  return new_buffer;
}