#include "Image.h"

#include <EASTL/memory.h>

#include <cassert>

#include "Tools.h"

void blu::core::Image::Destroy(const VkDevice& device,
                               const VmaAllocator& allocator) {
  if (sampler) {
    vkDestroySampler(device, sampler, nullptr);
  }

  if (view) {
    vkDestroyImageView(device, view, nullptr);
  }

  if (image) {
    vmaDestroyImage(allocator, image, alloc);
  }
}

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
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
  };

  VmaAllocationCreateInfo vma_create_info{
      .flags = flags,
      .requiredFlags = required_flags,
  };

  VmaAllocationInfo alloc_info;
  VK_CHECK_RESULT(vmaCreateImage(allocator, &image_create_info,
                                 &vma_create_info, &new_image->image,
                                 &new_image->alloc, &alloc_info));
  new_image->size = alloc_info.size;
  new_image->mip_levels = mip_levels;
  new_image->layout = VK_IMAGE_LAYOUT_UNDEFINED;

  return new_image;
}

blu::core::Image* blu::core::Image::CreateImage(
    const VkDevice& device, const VmaAllocator& allocator,
    VkCommandBuffer cmd_buf, uint32_t src_queue, uint32_t dst_queue,
    VkFormat format, uint32_t width, uint32_t height,
    VkSampleCountFlagBits samples, VkImageTiling tiling,
    VkImageUsageFlags usage, VkMemoryPropertyFlags required_flags,
    unsigned char* data, uint32_t data_size, blu::core::Buffer*& stg_buffer,
    VmaAllocationCreateFlags flags) {
  blu::core::Image* new_image = CreateImage(
      device, allocator, format, width, height,
      static_cast<uint32_t>(floor(log2(std::max(width, height))) + 1), samples,
      tiling, usage, required_flags, flags);

  VkImageSubresourceRange img_range{
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .baseMipLevel = 0,
      .levelCount = 1,
      .baseArrayLayer = 0,
      .layerCount = VK_REMAINING_ARRAY_LAYERS,
  };

  blu::core::Image::ImageLayoutTransition(
      cmd_buf, new_image->image, VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, img_range, src_queue, dst_queue);

  stg_buffer = blu::core::Buffer::CreateBuffer(
      device, allocator, new_image->size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      VMA_ALLOCATION_CREATE_MAPPED_BIT);

  memcpy(stg_buffer->mapped_data, data, data_size);

  VkBufferImageCopy region{
      .bufferOffset = 0,
      .bufferRowLength = 0,
      .bufferImageHeight = 0,
      .imageSubresource = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                           .mipLevel = 0,
                           .baseArrayLayer = 0,
                           .layerCount = 1},
      .imageOffset = {0, 0, 0},
      .imageExtent = {width, height, 1},
  };

  vkCmdCopyBufferToImage(cmd_buf, stg_buffer->buffer, new_image->image,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

  blu::core::Image::ImageLayoutTransition(
      cmd_buf, new_image->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, img_range, src_queue, dst_queue);

  for (size_t i = 1; i < new_image->mip_levels; i++) {
    VkImageBlit blit{
        .srcSubresource{
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel = static_cast<uint32_t>(i - 1),
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
        .srcOffsets{{0, 0, 0},
                    {int32_t(width >> (i - 1)), int32_t(height >> (i - 1)), 1}},
        .dstSubresource{
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel = static_cast<uint32_t>(i),
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
        .dstOffsets{{0, 0, 0}, {int32_t(width >> i), int32_t(height >> i), 1}},
    };
    img_range.baseMipLevel = i;

    blu::core::Image::ImageLayoutTransition(
        cmd_buf, new_image->image, VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, img_range, src_queue, dst_queue);

    vkCmdBlitImage(cmd_buf, new_image->image,
                   VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, new_image->image,
                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit,
                   VK_FILTER_LINEAR);

    blu::core::Image::ImageLayoutTransition(
        cmd_buf, new_image->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, img_range, src_queue, dst_queue);
  }
  blu::core::Image::ImageLayoutTransition(
      cmd_buf, new_image->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, img_range, src_queue,
      dst_queue);

  return new_image;
}

void blu::core::Image::CreateImageView(
    const VkDevice& device, blu::core::Image* image, VkFormat format,
    VkImageSubresourceRange const& subresource_range,
    VkImageViewType image_view_type) {
  VkImageViewCreateInfo image_view_info{
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .pNext = nullptr,
      .image = image->image,
      .viewType = image_view_type,
      .format = format,
      .subresourceRange = subresource_range,
  };

  vkCreateImageView(device, &image_view_info, nullptr, &image->view);
}

void blu::core::Image::CreateImageSampler(
    const VkDevice& device,
    const VkPhysicalDeviceProperties& physical_device_properties,
    blu::core::Image* image) {
  VkSamplerCreateInfo sampler_create_info{
      .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
      .flags = 0,
      .magFilter = VK_FILTER_LINEAR,
      .minFilter = VK_FILTER_LINEAR,
      .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
      .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
      .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
      .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
      .mipLodBias = 0.0f,
      .anisotropyEnable = VK_TRUE,
      .maxAnisotropy = physical_device_properties.limits.maxSamplerAnisotropy,
      .compareEnable = VK_FALSE,
      .compareOp = VK_COMPARE_OP_ALWAYS,
      .minLod = 0.0f,
      .maxLod = static_cast<float>(image->mip_levels),
      .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
      .unnormalizedCoordinates = VK_FALSE,
  };

  vkCreateSampler(device, &sampler_create_info, nullptr, &image->sampler);
}

void blu::core::Image::ImageLayoutTransition(
    VkCommandBuffer command_buffer, VkImage image,
    VkPipelineStageFlags src_stage_mask, VkPipelineStageFlags dst_stage_mask,
    VkAccessFlags src_access_mask, VkAccessFlags dst_access_mask,
    VkImageLayout old_layout, VkImageLayout new_layout,
    const VkImageSubresourceRange& subresource_range, uint32_t src_queue_index,
    uint32_t dst_queue_index) {
  VkImageMemoryBarrier image_memory_barrier{
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      .pNext = nullptr,
      .srcAccessMask = src_access_mask,
      .dstAccessMask = dst_access_mask,
      .oldLayout = old_layout,
      .newLayout = new_layout,
      .srcQueueFamilyIndex = src_queue_index,
      .dstQueueFamilyIndex = dst_queue_index,
      .image = image,
      .subresourceRange = subresource_range,
  };
  vkCmdPipelineBarrier(command_buffer, src_stage_mask, dst_stage_mask, 0, 0,
                       nullptr, 0, nullptr, 1, &image_memory_barrier);
}

void blu::core::Image::ImageLayoutTransition(
    VkCommandBuffer command_buffer, VkImage image, VkImageLayout old_layout,
    VkImageLayout new_layout, const VkImageSubresourceRange& subresource_range,
    uint32_t src_queue_index, uint32_t dst_queue_index) {
  VkPipelineStageFlags src_stage_mask = GetPipelineStageFlags(old_layout);
  VkPipelineStageFlags dst_stage_mask = GetPipelineStageFlags(new_layout);
  VkAccessFlags src_access_mask = GetAccessFlags(old_layout);
  VkAccessFlags dst_access_mask = GetAccessFlags(new_layout);

  ImageLayoutTransition(command_buffer, image, src_stage_mask, dst_stage_mask,
                        src_access_mask, dst_access_mask, old_layout,
                        new_layout, subresource_range, src_queue_index,
                        dst_queue_index);
}

VkAccessFlags blu::core::Image::GetAccessFlags(VkImageLayout layout) {
  switch (layout) {
    case VK_IMAGE_LAYOUT_UNDEFINED:
    case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
      return 0;
    case VK_IMAGE_LAYOUT_PREINITIALIZED:
      return VK_ACCESS_HOST_WRITE_BIT;
    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
      return VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
             VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL:
      return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
             VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    case VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR:
      return VK_ACCESS_FRAGMENT_SHADING_RATE_ATTACHMENT_READ_BIT_KHR;
    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
      return VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
      return VK_ACCESS_TRANSFER_READ_BIT;
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
      return VK_ACCESS_TRANSFER_WRITE_BIT;
    case VK_IMAGE_LAYOUT_GENERAL:
      assert(false &&
             "Don't know how to get a meaningful VkAccessFlags for "
             "VK_IMAGE_LAYOUT_GENERAL! Don't use it!");
      return 0;
    default:
      assert(false);
      return 0;
  }
}

VkPipelineStageFlags blu::core::Image::GetPipelineStageFlags(
    VkImageLayout layout) {
  switch (layout) {
    case VK_IMAGE_LAYOUT_UNDEFINED:
      return VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    case VK_IMAGE_LAYOUT_PREINITIALIZED:
      return VK_PIPELINE_STAGE_HOST_BIT;
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
      return VK_PIPELINE_STAGE_TRANSFER_BIT;
    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
      return VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL:
      return VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
             VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    case VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR:
      return VK_PIPELINE_STAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR;
    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
      return VK_PIPELINE_STAGE_VERTEX_SHADER_BIT |
             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
      return VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    case VK_IMAGE_LAYOUT_GENERAL:
      assert(false &&
             "Don't know how to get a meaningful VkPipelineStageFlags for "
             "VK_IMAGE_LAYOUT_GENERAL! Don't use it!");
      return 0;
    default:
      assert(false && layout);
      return 0;
  }
}