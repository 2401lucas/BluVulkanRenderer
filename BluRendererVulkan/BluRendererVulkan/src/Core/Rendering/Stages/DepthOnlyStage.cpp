#include "DepthOnlyStage.h"

namespace blu::core::rendering {
DepthOnlyStage::DepthOnlyStage(Device* device, VmaAllocator allocator,
                               blu::core::rendering::Pipeline* pipeline,
                               eastl::vector<VkCommandPool>& pools,
                               uint32_t width, uint32_t height)
    : Stage(device, allocator, pipeline, pools) {
  Resize(pools.size(), width, height);
}
DepthOnlyStage::~DepthOnlyStage() {
  for (auto& img : depth_only_output_images) {
    img->Destroy(device_->GetLogicalDevice(), allocator_);
    delete img;
  }
}
void DepthOnlyStage::Resize(uint32_t frame_count, uint32_t width,
                            uint32_t height) {
  for (auto& img : depth_only_output_images) {
    img->Destroy(device_->GetLogicalDevice(), allocator_);
    delete img;
  }
  width_ = width;
  height_ = height;
  depth_only_output_images.resize(frame_count);
  for (size_t i = 0; i < frame_count; i++) {
    depth_only_output_images[i] = blu::core::Image::CreateImage(
        device_->GetLogicalDevice(), allocator_, DEPTH_FORMAT, width_, height_,
        1, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
            VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    VkImageSubresourceRange depth_range{
        .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
        .baseMipLevel = 0,
        .levelCount = VK_REMAINING_MIP_LEVELS,
        .baseArrayLayer = 0,
        .layerCount = VK_REMAINING_ARRAY_LAYERS,
    };

    blu::core::Image::CreateImageView(device_->GetLogicalDevice(),
                                      depth_only_output_images[i],
                                      DEPTH_FORMAT, depth_range);
  }
}

void DepthOnlyStage::Run(uint32_t frame_index, Buffer* draw_command_buffer,
                         eastl::vector<VkDescriptorSet> descriptor_sets,
                         Buffer* vertex_buffer, Buffer* index_buffer,
                         VkSemaphore wait_semaphore, uint64_t wait_value,
                         VkPipelineStageFlags wait_flag,
                         VkSemaphore signal_semaphore, uint64_t signal_value,
                         VkFence fence) {
  auto depth_only_buf = Begin(frame_index);
  VkImageSubresourceRange depth_range{
      .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
      .baseMipLevel = 0,
      .levelCount = 1,
      .baseArrayLayer = 0,
      .layerCount = 1,
  };

  blu::core::Image::ImageLayoutTransition(
      depth_only_buf, depth_only_output_images[frame_index]->image,
      VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
      depth_range);

  VkClearValue depth_value = {1.0f, 0};
  VkRenderingAttachmentInfo depth_attachment_info{
      .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
      .imageView = depth_only_output_images[frame_index]->view,
      .imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
      .resolveMode = VK_RESOLVE_MODE_NONE,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
      .clearValue = depth_value,
  };
  VkRenderingInfo depth_only_render_info{
      .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
      .renderArea = {{.x = 0, .y = 0}, width_, height_},
      .layerCount = 1,
      .viewMask = 0,
      .colorAttachmentCount = 0,
      .pDepthAttachment = &depth_attachment_info,
  };

  vkCmdBeginRendering(depth_only_buf, &depth_only_render_info);
  VkViewport viewport{
      .width = static_cast<float>(width_),
      .height = static_cast<float>(height_),
      .minDepth = 0.0f,
      .maxDepth = 1.0f,
  };
  vkCmdSetViewport(depth_only_buf, 0, 1, &viewport);

  VkRect2D scissor{
      .offset{.x = 0, .y = 0},
      .extent{
          .width = width_,
          .height = height_,
      },
  };
  vkCmdSetScissor(depth_only_buf, 0, 1, &scissor);

  vkCmdBindPipeline(depth_only_buf, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    *pipeline_->GetPipeline());

  vkCmdBindDescriptorSets(depth_only_buf, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          *pipeline_->GetPipelineLayout(), 0,
                          descriptor_sets.size(), descriptor_sets.data(), 0,
                          nullptr);

  VkDeviceSize offsets[1] = {0};
  vkCmdBindVertexBuffers(depth_only_buf, 0, 1, &vertex_buffer->buffer, offsets);
  vkCmdBindIndexBuffer(depth_only_buf, index_buffer->buffer, 0,
                       VK_INDEX_TYPE_UINT32);

  vkCmdDrawIndexedIndirectCount(depth_only_buf, draw_command_buffer->buffer,
                                sizeof(uint32_t), draw_command_buffer->buffer,
                                0, MAX_MODELS, DRAW_COMMAND_BUFFER_SIZE);

  vkCmdEndRendering(depth_only_buf);

  blu::core::Image::ImageLayoutTransition(
      depth_only_buf, depth_only_output_images[frame_index]->image,
      VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, depth_range);
  End(frame_index);


  VkTimelineSemaphoreSubmitInfo timeline_semaphore_values{
      .sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO,
  };

  VkSubmitInfo depth_only_info{
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .commandBufferCount = 1,
      .pCommandBuffers = &depth_only_buf,
  };

  if (wait_semaphore != VK_NULL_HANDLE) {
    depth_only_info.waitSemaphoreCount = 1;
    depth_only_info.pWaitSemaphores = &wait_semaphore;
    depth_only_info.pWaitDstStageMask = &wait_flag;
    if (wait_value != UINT64_MAX) {
      timeline_semaphore_values.waitSemaphoreValueCount = 1;
      timeline_semaphore_values.pWaitSemaphoreValues = &wait_value;
      depth_only_info.pNext = &timeline_semaphore_values;
    }
  }

  if (signal_semaphore != VK_NULL_HANDLE) {
    depth_only_info.signalSemaphoreCount = 1;
    depth_only_info.pSignalSemaphores = &signal_semaphore;
    if (signal_value != UINT64_MAX) {
      timeline_semaphore_values.signalSemaphoreValueCount = 1;
      timeline_semaphore_values.pSignalSemaphoreValues = &signal_value;
      depth_only_info.pNext = &timeline_semaphore_values;
    }
  }

  VK_CHECK_RESULT(
      vkQueueSubmit(device_->queues.graphics, 1, &depth_only_info, fence));
  // This is used for debug output
  depth_only_output_images[frame_index]->layout =
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
}
}  // namespace blu::core::rendering