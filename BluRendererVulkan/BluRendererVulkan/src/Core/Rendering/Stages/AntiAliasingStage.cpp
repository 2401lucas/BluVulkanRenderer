#include "AntiAliasingStage.h"

namespace blu::core::rendering::stage {
AntiAliasingStage::AntiAliasingStage(
    Device* device, eastl::vector<VkDescriptorSetLayout> descriptor_set_layouts,
    VkPipelineShaderStageCreateInfo shader_infos)
    : Stage(device, nullptr) {
  blu::core::rendering::ComputePipelineCreateInfo create_info{
      .descriptor_set_layouts = descriptor_set_layouts,
      .shader = shader_infos,
      .push_const = {VkPushConstantRange(VK_SHADER_STAGE_COMPUTE_BIT, 0,
                                         sizeof(AntiAliasingPushConst))},
  };

  pipeline_ = new blu::core::rendering::Pipeline(device_, create_info);
}

AntiAliasingStage::~AntiAliasingStage() { delete pipeline_; }

void AntiAliasingStage::Run(VkCommandBuffer buf,
                            eastl::vector<VkDescriptorSet> descriptor_sets,
                            uint32_t width, uint32_t height, Image* input,
                            Image* output) {
  VkImageSubresourceRange range{
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .baseMipLevel = 0,
      .levelCount = 1,
      .baseArrayLayer = 0,
      .layerCount = 1,
  };
  blu::core::Image::ImageLayoutTransition(
      buf, input->image, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
      VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_SHADER_WRITE_BIT,
      VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, range);
  blu::core::Image::ImageLayoutTransition(
      buf, output->image, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
      VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_SHADER_READ_BIT,
      VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_GENERAL, range);

  vkCmdBindDescriptorSets(
      buf, VK_PIPELINE_BIND_POINT_COMPUTE, *pipeline_->GetPipelineLayout(), 0,
      descriptor_sets.size(), descriptor_sets.data(), 0, nullptr);

  uint32_t workgroupSizeX = (width + 7) / 8;
  uint32_t workgroupSizeY = (height + 7) / 8;

  AntiAliasingPushConst push_const{.width = width, .height = height};
  vkCmdPushConstants(buf, *pipeline_->GetPipelineLayout(),
                     VK_SHADER_STAGE_COMPUTE_BIT, 0,
                     sizeof(AntiAliasingPushConst), &push_const);

  vkCmdBindPipeline(buf, VK_PIPELINE_BIND_POINT_COMPUTE,
                    *pipeline_->GetPipeline());

  vkCmdDispatch(buf, workgroupSizeX, workgroupSizeY, 1);
}
}  // namespace blu::core::rendering::stage