#include "Stage.h"

namespace blu::core::rendering {
Stage::Stage(Device* device, VmaAllocator allocator,
             blu::core::rendering::Pipeline* pipeline,
             eastl::vector<VkCommandPool>& pools) {
  device_ = device;
  pipeline_ = pipeline;
  allocator_ = allocator;

  VkCommandBufferAllocateInfo command_buffer_alloc_info{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .pNext = nullptr,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = 1,
  };

  auto frame_count = pools.size();
  command_buffers.resize(frame_count);

  for (size_t i = 0; i < frame_count; i++) {
    command_buffer_alloc_info.commandPool = pools[i];
    vkAllocateCommandBuffers(device_->GetLogicalDevice(),
                             &command_buffer_alloc_info, &command_buffers[i]);
  }
}

Stage::~Stage() {
  if (pipeline_ != nullptr) {
    pipeline_->~Pipeline();
    delete pipeline_;
  }
}

VkCommandBuffer Stage::Begin(uint32_t index) {
  auto cmd_buf = command_buffers[index];
  vkResetCommandBuffer(cmd_buf, 0);

  VkCommandBufferBeginInfo cmd_buf_begin{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .pNext = nullptr,
      .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
      .pInheritanceInfo = nullptr,
  };

  vkBeginCommandBuffer(cmd_buf, &cmd_buf_begin);
  return cmd_buf;
}

void Stage::End(uint32_t index) { vkEndCommandBuffer(command_buffers[index]); }

VkSubmitInfo Stage::PrepareSubmitInfo(
    VkTimelineSemaphoreSubmitInfo& timeline_semaphore_values,
    VkSemaphore& wait_semaphore, uint64_t& wait_value,
    VkPipelineStageFlags& wait_flag, VkSemaphore& signal_semaphore,
    uint64_t& signal_value) {
  timeline_semaphore_values = {
      .sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO};

  VkSubmitInfo submit_info{
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
  };

  if (wait_semaphore != VK_NULL_HANDLE) {
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &wait_semaphore;
    submit_info.pWaitDstStageMask = &wait_flag;
    if (wait_value != UINT64_MAX) {
      timeline_semaphore_values.waitSemaphoreValueCount = 1;
      timeline_semaphore_values.pWaitSemaphoreValues = &wait_value;
      submit_info.pNext = &timeline_semaphore_values;
    }
  }

  if (signal_semaphore != VK_NULL_HANDLE) {
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &signal_semaphore;
    if (signal_value != UINT64_MAX) {
      timeline_semaphore_values.signalSemaphoreValueCount = 1;
      timeline_semaphore_values.pSignalSemaphoreValues = &signal_value;
      submit_info.pNext = &timeline_semaphore_values;
    }
  }

  return submit_info;
}
}  // namespace blu::core::rendering
