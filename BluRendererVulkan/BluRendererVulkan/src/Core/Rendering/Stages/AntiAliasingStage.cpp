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
                            uint32_t width, uint32_t height) {
  vkCmdBindDescriptorSets(
      buf, VK_PIPELINE_BIND_POINT_COMPUTE, *pipeline_->GetPipelineLayout(), 0,
      descriptor_sets.size(), descriptor_sets.data(), 0, nullptr);

  uint32_t workgroupSizeX = (width + 7) / 8;
  uint32_t workgroupSizey = (height + 7) / 8;

  AntiAliasingPushConst push_const{.width = width, .height = height};
  vkCmdPushConstants(buf, *pipeline_->GetPipelineLayout(),
                     VK_SHADER_STAGE_COMPUTE_BIT, 0,
                     sizeof(AntiAliasingPushConst), &push_const);

  vkCmdDispatch(buf, workgroupSizeX, workgroupSizey, 1);
}
}  // namespace blu::core::rendering::stage