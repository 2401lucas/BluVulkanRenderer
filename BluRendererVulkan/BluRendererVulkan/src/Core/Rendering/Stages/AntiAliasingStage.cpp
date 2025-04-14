#include "AntiAliasingStage.h"

namespace blu::core::rendering {
AntiAliasingStage::AntiAliasingStage(Device* device, VmaAllocator allocator,
                                     blu::core::rendering::Pipeline* pipeline,
                                     eastl::vector<VkCommandPool>& pools,
                                     uint32_t width, uint32_t height)
    : Stage(device, allocator, pipeline, pools) {
  Resize(pools.size(), width, height);
}

AntiAliasingStage::~AntiAliasingStage() {
  for (auto& img : aliased_output_images) {
    img->Destroy(device_->GetLogicalDevice(), allocator_);
    delete img;
  }
}

void AntiAliasingStage::Resize(uint32_t frame_count, uint32_t width,
                               uint32_t height) {
  width_ = width;
  height_ = height;

  for (auto& img : aliased_output_images) {
    img->Destroy(device_->GetLogicalDevice(), allocator_);
    delete img;
  }

  aliased_output_images.resize(frame_count);
  for (size_t i = 0; i < frame_count; i++) {
    aliased_output_images[i] = blu::core::Image::CreateImage(
        device_->GetLogicalDevice(), allocator_, COLOR_FORMAT, width_, height_,
        1, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    VkImageSubresourceRange range{
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel = 0,
        .levelCount = VK_REMAINING_MIP_LEVELS,
        .baseArrayLayer = 0,
        .layerCount = VK_REMAINING_ARRAY_LAYERS,
    };

    blu::core::Image::CreateImageView(device_->GetLogicalDevice(),
                                      aliased_output_images[i], COLOR_FORMAT,
                                      range);
  }
}

void AntiAliasingStage::Run(uint32_t frame_index, VkSemaphore wait_semaphore,
                            uint64_t wait_value, VkPipelineStageFlags wait_flag,
                            VkSemaphore signal_semaphore, uint64_t signal_value,
                            VkFence fence) {
  auto aa_buf = Begin(frame_index);

  VkImageSubresourceRange range{
      .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
      .baseMipLevel = 0,
      .levelCount = 1,
      .baseArrayLayer = 0,
      .layerCount = 1,
  };

  blu::core::Image::ImageLayoutTransition(
      aa_buf, aliased_output_images[frame_index]->image,
      VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, range);

  vkCmdDispatch(aa_buf, (width_ + 7) / 8, (height_ + 7) / 8, 1);
  End(frame_index);

  VkSubmitInfo aa_info = PrepareSubmitInfo(
      wait_semaphore, wait_value, wait_flag, signal_semaphore, signal_value);

  aa_info.commandBufferCount = 1;
  aa_info.pCommandBuffers = &aa_buf;

  VK_CHECK_RESULT(vkQueueSubmit(device_->queues.compute, 1, &aa_info, fence));
}
}  // namespace blu::core::rendering