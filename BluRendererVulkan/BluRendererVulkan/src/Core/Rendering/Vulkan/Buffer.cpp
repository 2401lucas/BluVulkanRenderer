#include "Buffer.h"

blu::core::Buffer* CreateBuffer(const VkLogicalDevice& device, const VmaAllocator& allocator, VkDeviceSize size, VkBufferUsageFlags usage, 
    VkMemoryPropertyFlags required_flags, VmaAllocationCreateFlags flags = 0){
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

    VmaAllocationInfo allocInfo;
    vmaCreateBuffer(allocator, &buf_ci, &alloc_ci,
                    &new_buffer->buffer,
                    &new_buffer->alloc, &allocInfo);

    new_buffer->size = buf_ci.size;
    new_buffer->mapped_data = allocInfo.pMappedData;

    if(usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) {
    VkBufferDeviceAddressInfo info{
        .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        .buffer = new_buffer->buffer};

    new_buffer->device_address =
        vkGetBufferDeviceAddress(device, &info);
    }

  return new_buffer;
}