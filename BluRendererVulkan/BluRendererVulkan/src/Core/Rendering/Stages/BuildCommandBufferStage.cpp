#include "BuildCommandBufferStage.h"

namespace blu::core::rendering {
BuildCommandBufferStage::BuildCommandBufferStage(
    Device* device, VkPipelineShaderStageCreateInfo shader_create_info)
    : Stage(device, nullptr) {
  blu::core::rendering::ComputePipelineCreateInfo
      build_command_buffer_create_info{
          .shader = shader_create_info,
          .push_const = {VkPushConstantRange(
              VK_SHADER_STAGE_COMPUTE_BIT, 0,
              sizeof(BuildCommandBufferPushConst))},
      };

  pipeline_ = new blu::core::rendering::Pipeline(
      device_, build_command_buffer_create_info);
}

BuildCommandBufferStage::~BuildCommandBufferStage() { delete pipeline_; }

void BuildCommandBufferStage::Run(VkCommandBuffer buf, uint32_t frame_index,
                                  BufferInfo model_data, BufferInfo models,
                                  uint32_t model_count, Buffer* output_buffer) {
  vkCmdFillBuffer(buf, output_buffer->buffer, 0, sizeof(uint32_t), 0);

  Buffer::BufferMemoryBarrier(
      buf, output_buffer->buffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
      VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
      VK_ACCESS_SHADER_WRITE_BIT);

  vkCmdBindPipeline(buf, VK_PIPELINE_BIND_POINT_COMPUTE,
                    *pipeline_->GetPipeline());

  BuildCommandBufferPushConst push_const{
      .model_data = model_data,
      .models = models,
      .culled_output = output_buffer->device_address,
      .model_count = model_count,
  };

  vkCmdPushConstants(buf, *pipeline_->GetPipelineLayout(),
                     VK_SHADER_STAGE_COMPUTE_BIT, 0,
                     sizeof(BuildCommandBufferPushConst), &push_const);

  uint32_t workgroupSizeX = (model_count + 127) / 128;

  vkCmdDispatch(buf, workgroupSizeX, 1, 1);
}
}  // namespace blu::core::rendering