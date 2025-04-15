#include "OpaqueRenderStage.h"

#include <EASTL/array.h>

namespace blu::core::rendering {
OpaqueRenderStage::OpaqueRenderStage(Device* device, VmaAllocator allocator,
                                     blu::core::rendering::Pipeline* pipeline,
                                     eastl::vector<VkCommandPool>& pools,
                                     uint32_t width, uint32_t height)
    : Stage(device, allocator, pipeline, pools) {
  Resize(pools.size(), width, height);
}

OpaqueRenderStage::~OpaqueRenderStage() {
  for (auto& img : color_output_images) {
    img->Destroy(device_->GetLogicalDevice(), allocator_);
    delete img;
  }
  depth_stencil_image_->Destroy(device_->GetLogicalDevice(), allocator_);
  delete depth_stencil_image_;
}

void OpaqueRenderStage::Resize(uint32_t frame_count, uint32_t width,
                               uint32_t height) {
  width_ = width;
  height_ = height;
  if (depth_stencil_image_ != nullptr) {
    depth_stencil_image_->Destroy(device_->GetLogicalDevice(), allocator_);
    delete depth_stencil_image_;
  }

  for (auto& img : color_output_images) {
    img->Destroy(device_->GetLogicalDevice(), allocator_);
    delete img;
  }

  depth_stencil_image_ = blu::core::Image::CreateImage(
      device_->GetLogicalDevice(), allocator_, DEPTH_FORMAT, width_, height_, 1,
      VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TILING_OPTIMAL,
      VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  VkImageSubresourceRange depth_range{
      .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
      .baseMipLevel = 0,
      .levelCount = VK_REMAINING_MIP_LEVELS,
      .baseArrayLayer = 0,
      .layerCount = VK_REMAINING_ARRAY_LAYERS,
  };

  blu::core::Image::CreateImageView(device_->GetLogicalDevice(),
                                    depth_stencil_image_, DEPTH_FORMAT,
                                    depth_range);

  color_output_images.resize(frame_count);
  for (size_t i = 0; i < frame_count; i++) {
    color_output_images[i] = blu::core::Image::CreateImage(
        device_->GetLogicalDevice(), allocator_, COLOR_FORMAT, width_, height_,
        1, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    VkImageSubresourceRange range{
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel = 0,
        .levelCount = VK_REMAINING_MIP_LEVELS,
        .baseArrayLayer = 0,
        .layerCount = VK_REMAINING_ARRAY_LAYERS,
    };

    blu::core::Image::CreateImageView(device_->GetLogicalDevice(),
                                      color_output_images[i], COLOR_FORMAT,
                                      range);
  }
}

void OpaqueRenderStage::Run(uint32_t frame_index, Buffer* draw_command_buffer,
                            eastl::vector<VkDescriptorSet> descriptor_sets,
                            Buffer* vertex_buffer, Buffer* normal_buffer,
                            Buffer* uv_buffer, Buffer* index_buffer,
                            VkSemaphore wait_semaphore, uint64_t wait_value,
                            VkPipelineStageFlags wait_flag,
                            VkSemaphore signal_semaphore, uint64_t signal_value,
                            VkFence fence) {
  auto opaque_render_buf = Begin(frame_index);

  VkImageSubresourceRange range{
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .baseMipLevel = 0,
      .levelCount = 1,
      .baseArrayLayer = 0,
      .layerCount = 1,
  };

  VkImageSubresourceRange depth_range{
      .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
      .baseMipLevel = 0,
      .levelCount = 1,
      .baseArrayLayer = 0,
      .layerCount = 1,
  };

  blu::core::Image::ImageLayoutTransition(
      opaque_render_buf, color_output_images[frame_index]->image,
      VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      range);

  blu::core::Image::ImageLayoutTransition(
      opaque_render_buf, depth_stencil_image_->image, VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, depth_range);

  eastl::array<VkClearValue, 2> clear_values{};
  clear_values[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
  clear_values[1].depthStencil = {1.0f, 0};

  VkRenderingAttachmentInfo color_attachment_info{
      .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
      .imageView = color_output_images[frame_index]->view,
      .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      .resolveMode = VK_RESOLVE_MODE_NONE,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
      .clearValue = clear_values[0],
  };

  VkRenderingAttachmentInfo depth_attachment_info{
      .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
      .imageView = depth_stencil_image_->view,
      .imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
      .resolveMode = VK_RESOLVE_MODE_NONE,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
      .clearValue = clear_values[1],
  };
  VkRenderingInfo render_info{
      .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
      .renderArea = {{.x = 0, .y = 0}, width_, height_},
      .layerCount = 1,
      .viewMask = 0,
      .colorAttachmentCount = 1,
      .pColorAttachments = &color_attachment_info,
      .pDepthAttachment = &depth_attachment_info,
      //.pStencilAttachment = &depth_attachment_info,
  };

  vkCmdBeginRendering(opaque_render_buf, &render_info);

  VkViewport viewport{
      .width = static_cast<float>(width_),
      .height = static_cast<float>(height_),
      .minDepth = 0.0f,
      .maxDepth = 1.0f,
  };

  vkCmdSetViewport(opaque_render_buf, 0, 1, &viewport);

  VkRect2D scissor{
      .offset{.x = 0, .y = 0},
      .extent{
          .width = width_,
          .height = height_,
      },
  };

  vkCmdSetScissor(opaque_render_buf, 0, 1, &scissor);
  VkDeviceSize offsets[1] = {0};

  vkCmdBindPipeline(opaque_render_buf, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    *pipeline_->GetPipeline());

  vkCmdBindDescriptorSets(opaque_render_buf, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          *pipeline_->GetPipelineLayout(), 0,
                          descriptor_sets.size(), descriptor_sets.data(), 0,
                          nullptr);

  vkCmdBindVertexBuffers(opaque_render_buf, 0, 1, &vertex_buffer->buffer,
                         offsets);
  vkCmdBindVertexBuffers(opaque_render_buf, 1, 1, &normal_buffer->buffer,
                         offsets);
  vkCmdBindVertexBuffers(opaque_render_buf, 2, 1, &uv_buffer->buffer, offsets);

  vkCmdBindIndexBuffer(opaque_render_buf, index_buffer->buffer, 0,
                       VK_INDEX_TYPE_UINT32);

  vkCmdDrawIndexedIndirectCount(opaque_render_buf, draw_command_buffer->buffer,
                                sizeof(uint32_t), draw_command_buffer->buffer,
                                0, MAX_MODELS, DRAW_COMMAND_BUFFER_SIZE);

  vkCmdEndRendering(opaque_render_buf);

  // This is used for debug output
  color_output_images[frame_index]->layout =
      VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  End(frame_index);

  VkTimelineSemaphoreSubmitInfo timeline_info;
  VkSubmitInfo submit_info =
      PrepareSubmitInfo(timeline_info, wait_semaphore, wait_value, wait_flag,
                        signal_semaphore, signal_value);
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &opaque_render_buf;

  VK_CHECK_RESULT(
      vkQueueSubmit(device_->queues.graphics, 1, &submit_info, fence));
}
}  // namespace blu::core::rendering