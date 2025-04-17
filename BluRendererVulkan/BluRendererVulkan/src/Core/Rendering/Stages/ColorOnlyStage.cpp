#include "ColorOnlyStage.h"

#include <EASTL/array.h>

namespace blu::core::rendering::stage {
ColorOnlyStage::ColorOnlyStage(
    Device* device, eastl::vector<VkDescriptorSetLayout> descriptor_set_layouts,
    eastl::vector<VkPipelineShaderStageCreateInfo> shader_infos)
    : Stage(device, nullptr) {
  blu::core::rendering::GraphicsPipelineCreateInfo pipeline_create_info{
      .descriptor_set_layouts = descriptor_set_layouts,
      .input_assembly_flags = 0,
      .input_assembly_topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
      .input_assembly_primitive_restart_enable = VK_FALSE,
      .rasteriazation_flags = 0,
      .rasteriazation_state_polygone_mode = VK_POLYGON_MODE_FILL,
      .rasteriazation_state_cull_mode = VK_CULL_MODE_NONE,
      .rasteriazation_state_front_face = VK_FRONT_FACE_COUNTER_CLOCKWISE,
      .color_blend_attachment_states = {{
          .blendEnable = VK_TRUE,
          .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
          .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
          .colorBlendOp = VK_BLEND_OP_ADD,
          .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
          .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
          .alphaBlendOp = VK_BLEND_OP_ADD,
          .colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                            VK_COLOR_COMPONENT_G_BIT |
                            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
      }},
      .depth_stencil_depth_test = VK_FALSE,
      .depth_stencil_depth_write = VK_FALSE,
      .viewport_count = 1,
      .scissor_count = 1,
      .multisample_flags = 0,
      .multisample_count = VK_SAMPLE_COUNT_1_BIT,
      .dynamic_state_flags = 0,
      .dynamic_state_enables = {VK_DYNAMIC_STATE_VIEWPORT,
                                VK_DYNAMIC_STATE_SCISSOR},
      .vertex_input_bindings = {},
      .vertex_input_attributes = {},
      .color_attachment_formats = {COLOR_FORMAT},
      .shaders = shader_infos,
  };

  pipeline_ = new blu::core::rendering::Pipeline(device_, pipeline_create_info);
}

ColorOnlyStage::~ColorOnlyStage() { delete pipeline_; }

void ColorOnlyStage::Run(VkCommandBuffer buf,
                         eastl::vector<VkDescriptorSet> descriptor_sets,
                         Image* output, uint32_t width, uint32_t height) {
  VkImageSubresourceRange range{
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .baseMipLevel = 0,
      .levelCount = 1,
      .baseArrayLayer = 0,
      .layerCount = 1,
  };
  blu::core::Image::ImageLayoutTransition(
      buf, output->image, VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, range);

  eastl::array<VkClearValue, 1> clear_values{};
  clear_values[0].color = {{0.0f, 0.0f, 0.0f, 0.0f}};

  VkRenderingAttachmentInfo color_attachment_info{
      .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
      .imageView = output->view,
      .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      .resolveMode = VK_RESOLVE_MODE_NONE,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
      .clearValue = clear_values[0],
  };

  VkRenderingInfo render_info{
      .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
      .renderArea = {{.x = 0, .y = 0}, width, height},
      .layerCount = 1,
      .viewMask = 0,
      .colorAttachmentCount = 1,
      .pColorAttachments = &color_attachment_info,
  };
  vkCmdBeginRendering(buf, &render_info);

  VkViewport viewport{
      .width = static_cast<float>(width),
      .height = static_cast<float>(height),
      .minDepth = 0.0f,
      .maxDepth = 1.0f,
  };

  vkCmdSetViewport(buf, 0, 1, &viewport);

  VkRect2D scissor{
      .offset{.x = 0, .y = 0},
      .extent{
          .width = width,
          .height = height,
      },
  };

  vkCmdSetScissor(buf, 0, 1, &scissor);
  VkDeviceSize offsets[1] = {0};

  vkCmdBindPipeline(buf, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    *pipeline_->GetPipeline());

  vkCmdBindDescriptorSets(
      buf, VK_PIPELINE_BIND_POINT_GRAPHICS, *pipeline_->GetPipelineLayout(), 0,
      descriptor_sets.size(), descriptor_sets.data(), 0, nullptr);

  vkCmdDraw(buf, 3, 1, 0, 0);

  vkCmdEndRendering(buf);
}
}  // namespace blu::core::rendering
