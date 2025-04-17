#include "DepthOnlyStage.h"

namespace blu::core::rendering::stage {
DepthOnlyStage::DepthOnlyStage(
    Device* device, eastl::vector<VkDescriptorSetLayout> descriptor_set_layouts,
    VkPipelineShaderStageCreateInfo shader_create_infos)
    : Stage(device, nullptr) {
  blu::core::rendering::GraphicsPipelineCreateInfo depth_only_stage_create_info{
      .descriptor_set_layouts = descriptor_set_layouts,
      .input_assembly_flags = 0,
      .input_assembly_topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
      .input_assembly_primitive_restart_enable = VK_FALSE,
      .rasteriazation_flags = 0,
      .rasteriazation_state_polygone_mode = VK_POLYGON_MODE_FILL,
      .rasteriazation_state_cull_mode = VK_CULL_MODE_FRONT_BIT,
      .rasteriazation_state_front_face = VK_FRONT_FACE_COUNTER_CLOCKWISE,
      .color_blend_attachment_states = {},
      .depth_stencil_depth_test = VK_TRUE,
      .depth_stencil_depth_write = VK_TRUE,
      .depth_stencil_depth_compare_op = VK_COMPARE_OP_LESS,
      .depth_stencil_front_compare_op = VK_COMPARE_OP_ALWAYS,
      .depth_stencil_back_compare_op = VK_COMPARE_OP_ALWAYS,
      .viewport_count = 1,
      .scissor_count = 1,
      .multisample_flags = 0,
      .multisample_count = VK_SAMPLE_COUNT_1_BIT,
      .dynamic_state_flags = 0,
      .dynamic_state_enables = {VK_DYNAMIC_STATE_VIEWPORT,
                                VK_DYNAMIC_STATE_SCISSOR},
      .vertex_input_bindings =
          {
              {0, 3 * sizeof(float), VK_VERTEX_INPUT_RATE_VERTEX},  // POS
          },
      .vertex_input_attributes =
          {
              {0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0},  // POS
          },
      .color_attachment_formats = {},
      .depth_format = DEPTH_FORMAT,

      .shaders{shader_create_infos},
  };

  pipeline_ =
      new blu::core::rendering::Pipeline(device_, depth_only_stage_create_info);
}
DepthOnlyStage::~DepthOnlyStage() { delete pipeline_; }

void DepthOnlyStage::Run(VkCommandBuffer buf, blu::core::Image* image,
                         uint32_t width, uint32_t height,
                         blu::core::Buffer* draw_command_buffer,
                         eastl::vector<VkDescriptorSet> descriptor_sets,
                         blu::core::Buffer* vertex_buffer,
                         blu::core::Buffer* index_buffer) {
  VkImageSubresourceRange depth_range{
      .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
      .baseMipLevel = 0,
      .levelCount = 1,
      .baseArrayLayer = 0,
      .layerCount = 1,
  };

  blu::core::Image::ImageLayoutTransition(
      buf, image->image, VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, depth_range);

  VkClearValue depth_value = {1.0f, 0};
  VkRenderingAttachmentInfo depth_attachment_info{
      .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
      .imageView = image->view,
      .imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
      .resolveMode = VK_RESOLVE_MODE_NONE,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
      .clearValue = depth_value,
  };
  VkRenderingInfo depth_only_render_info{
      .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
      .renderArea = {{.x = 0, .y = 0}, width, height},
      .layerCount = 1,
      .viewMask = 0,
      .colorAttachmentCount = 0,
      .pDepthAttachment = &depth_attachment_info,
  };

  vkCmdBeginRendering(buf, &depth_only_render_info);
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

  vkCmdBindPipeline(buf, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    *pipeline_->GetPipeline());

  vkCmdBindDescriptorSets(
      buf, VK_PIPELINE_BIND_POINT_GRAPHICS, *pipeline_->GetPipelineLayout(), 0,
      descriptor_sets.size(), descriptor_sets.data(), 0, nullptr);

  VkDeviceSize offsets[1] = {0};
  vkCmdBindVertexBuffers(buf, 0, 1, &vertex_buffer->buffer, offsets);
  vkCmdBindIndexBuffer(buf, index_buffer->buffer, 0, VK_INDEX_TYPE_UINT32);

  vkCmdDrawIndexedIndirectCount(buf, draw_command_buffer->buffer,
                                sizeof(uint32_t), draw_command_buffer->buffer,
                                0, MAX_MODELS, DRAW_COMMAND_BUFFER_SIZE);

  vkCmdEndRendering(buf);

  blu::core::Image::ImageLayoutTransition(
      buf, image->image, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, depth_range);
}
}  // namespace blu::core::rendering