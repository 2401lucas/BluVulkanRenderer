#include "BuildCommandBufferStage.h"

namespace blu::core::rendering {
BuildCommandBufferStage::BuildCommandBufferStage(
    Device* device, VmaAllocator allocator,
    blu::core::rendering::Pipeline* pipeline,
    eastl::vector<VkCommandPool>& pools)
    : Stage(device, allocator, pipeline, pools) {
  auto frame_count = pools.size();
  command_buffer_output_buffers_.resize(frame_count);
  // TEMP SIZE: TODO->DYNAMICBUFFERSIZE
  for (size_t i = 0; i < frame_count; i++) {
    command_buffer_output_buffers_[i] = blu::core::Buffer::CreateBuffer(
        device_->GetLogicalDevice(), allocator_,
        DRAW_COMMAND_BUFFER_SIZE * MAX_MODELS + sizeof(uint32_t),
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
            VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT |
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
            VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  }
}

BuildCommandBufferStage::~BuildCommandBufferStage() {
  for (auto& buf : command_buffer_output_buffers_) {
    buf->Destroy(allocator_);
    delete buf;
  }
}

void BuildCommandBufferStage::Run(uint32_t frame_index, BufferInfo model_data,
                                  BufferInfo models, uint32_t model_count,
                                  VkSemaphore wait_semaphore,
                                  uint64_t wait_value,
                                  VkPipelineStageFlags wait_flag,
                                  VkSemaphore signal_semaphore,
                                  uint64_t signal_value, VkFence fence) {
  auto build_command_buf = Begin(frame_index);

  vkCmdFillBuffer(build_command_buf,
                  command_buffer_output_buffers_[frame_index]->buffer, 0,
                  sizeof(uint32_t), 0);

  Buffer::BufferMemoryBarrier(
      build_command_buf, command_buffer_output_buffers_[frame_index]->buffer,
      VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
      VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_WRITE_BIT);

  vkCmdBindPipeline(build_command_buf, VK_PIPELINE_BIND_POINT_COMPUTE,
                    *pipeline_->GetPipeline());

  BuildCommandBufferPushConst push_const{
      .model_data = model_data,
      .models = models,
      .culled_output =
          command_buffer_output_buffers_[frame_index]->device_address,
      .model_count = model_count,
  };

  vkCmdPushConstants(build_command_buf, *pipeline_->GetPipelineLayout(),
                     VK_SHADER_STAGE_COMPUTE_BIT, 0,
                     sizeof(BuildCommandBufferPushConst), &push_const);

  uint32_t workgroupSizeX = (model_count + 127) / 128;

  vkCmdDispatch(build_command_buf, workgroupSizeX, 1, 1);
  End(frame_index);

  VkSubmitInfo submit_info = PrepareSubmitInfo(
      wait_semaphore, wait_value, wait_flag, signal_semaphore, signal_value);

  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &build_command_buf;

  VK_CHECK_RESULT(
      vkQueueSubmit(device_->queues.compute, 1, &submit_info, fence));
}
}  // namespace blu::core::rendering