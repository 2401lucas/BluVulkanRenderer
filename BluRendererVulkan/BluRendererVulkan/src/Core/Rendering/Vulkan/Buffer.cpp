#include "Buffer.h"
#include <cstring>

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
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
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
        .buffer = new_buffer->buffer,
    };

    new_buffer->device_address = vkGetBufferDeviceAddress(device, &info);
  }

  return new_buffer;
}

void blu::core::Buffer::BufferMemoryBarrier(
    VkCommandBuffer command_buffer, VkBuffer buffer,
    VkPipelineStageFlags2 src_stage_mask, VkPipelineStageFlags2 dst_stage_mask,
    VkAccessFlagBits2 src_access_mask, VkAccessFlagBits2 dst_access_mask,
    uint32_t src_queue_index, uint32_t dst_queue_index) {
  VkBufferMemoryBarrier2 buffer_memory_barrier{
      .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
      .srcStageMask = src_stage_mask,
      .srcAccessMask = src_access_mask,
      .dstStageMask = dst_stage_mask,
      .dstAccessMask = dst_access_mask,
      .srcQueueFamilyIndex = src_queue_index,
      .dstQueueFamilyIndex = dst_queue_index,
      .buffer = buffer,
      .offset = 0,
      .size = VK_WHOLE_SIZE,
  };

  VkDependencyInfoKHR dependency_info{
      .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO_KHR,
      .bufferMemoryBarrierCount = 1,
      .pBufferMemoryBarriers = &buffer_memory_barrier,
  };

  vkCmdPipelineBarrier2(command_buffer, &dependency_info);
}

void blu::core::Buffer::UploadToBuffer(
    const VkDevice& device, const VmaAllocator& allocator,
    blu::core::Buffer* dst_buffer, VkDeviceSize dst_offset,
    VkCommandBuffer copy_command, void* data, VkDeviceSize size,
    VkDeviceSize src_offset, blu::core::Buffer*& stg_buffer) {
  stg_buffer = blu::core::Buffer::CreateBuffer(
      device, allocator, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      VMA_ALLOCATION_CREATE_MAPPED_BIT);

  memcpy(stg_buffer->mapped_data, data, size);

  VkBufferCopy copy_region{
      .srcOffset = src_offset,
      .dstOffset = dst_offset,
      .size = size,
  };

  vkCmdCopyBuffer(copy_command, stg_buffer->buffer, dst_buffer->buffer, 1,
                  &copy_region);
}
