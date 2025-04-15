#include "ImageCopyStage.h"

namespace blu::core::rendering {
ImageCopyStage::ImageCopyStage(Device* device, VmaAllocator allocator,
                               eastl::vector<VkCommandPool>& pools)
    : Stage(device, allocator, nullptr, pools) {}

ImageCopyStage::~ImageCopyStage() {}

void ImageCopyStage::Run(uint32_t frame_index, VkImage src_img,
                         VkImageLayout src_image_layout, VkImage dst_img,
                         VkImageLayout dst_img_layout, uint32_t width,
                         uint32_t height, VkSemaphore wait_semaphore,
                         uint64_t wait_value, VkPipelineStageFlags wait_flag,
                         VkSemaphore signal_semaphore, uint64_t signal_value,
                         VkFence fence) {
  VkCommandBuffer copy_buffer = Begin(frame_index);

  VkImageSubresourceRange range{
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .baseMipLevel = 0,
      .levelCount = 1,
      .baseArrayLayer = 0,
      .layerCount = 1,
  };

  blu::core::Image::ImageLayoutTransition(
      copy_buffer, src_img, src_image_layout,
      VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, range);

  blu::core::Image::ImageLayoutTransition(
      copy_buffer, dst_img, VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, range);

  VkImageCopy img_cpy{
      .srcSubresource{
          .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
          .mipLevel = 0,
          .baseArrayLayer = 0,
          .layerCount = 1,
      },
      .srcOffset{0, 0, 0},
      .dstSubresource{
          .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
          .mipLevel = 0,
          .baseArrayLayer = 0,
          .layerCount = 1,
      },
      .dstOffset{0, 0, 0},
      .extent{.width = width, .height = height, .depth = 1},
  };

  vkCmdCopyImage(copy_buffer, src_img, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                 dst_img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &img_cpy);

  blu::core::Image::ImageLayoutTransition(copy_buffer, dst_img,
                                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                          dst_img_layout, range);

  End(frame_index);

  VkTimelineSemaphoreSubmitInfo timeline_info;
  VkSubmitInfo submit_info =
      PrepareSubmitInfo(timeline_info, wait_semaphore, wait_value, wait_flag,
                        signal_semaphore, signal_value);

  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &copy_buffer;

  VK_CHECK_RESULT(
      vkQueueSubmit(device_->queues.graphics, 1, &submit_info, fence));
}
}  // namespace blu::core::rendering