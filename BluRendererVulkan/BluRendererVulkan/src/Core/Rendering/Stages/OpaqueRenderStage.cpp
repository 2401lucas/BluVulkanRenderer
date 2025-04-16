#include "OpaqueRenderStage.h"

#include <EASTL/array.h>

namespace blu::core::rendering {
OpaqueRenderStage::OpaqueRenderStage(
    Device* device, eastl::vector<VkDescriptorSetLayout> descriptor_set_layouts,
    eastl::vector<VkPipelineShaderStageCreateInfo> shader_create_infos)
    : Stage(device, nullptr) {
  blu::core::rendering::GraphicsPipelineCreateInfo
      opaque_render_stage_create_info{
          .descriptor_set_layouts = descriptor_set_layouts,
          .input_assembly_flags = 0,
          .input_assembly_topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
          .input_assembly_primitive_restart_enable = VK_FALSE,
          .rasteriazation_flags = 0,
          .rasteriazation_state_polygone_mode = VK_POLYGON_MODE_FILL,
          .rasteriazation_state_cull_mode = VK_CULL_MODE_BACK_BIT,
          .rasteriazation_state_front_face = VK_FRONT_FACE_COUNTER_CLOCKWISE,
          .color_blend_attachment_states = {{
              .blendEnable = VK_FALSE,
              .colorWriteMask = 0xf /*RGBA*/,
          }},
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
                  {1, 3 * sizeof(float), VK_VERTEX_INPUT_RATE_VERTEX},  // NORM
                  {2, 3 * sizeof(float), VK_VERTEX_INPUT_RATE_VERTEX},  // UV
              },
          .vertex_input_attributes =
              {
                  {0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0},  // POS
                  {1, 1, VK_FORMAT_R32G32B32_SFLOAT, 0},  // NORM
                  {2, 2, VK_FORMAT_R32G32B32_SFLOAT, 0},  // UV
              },
          .color_attachment_formats = {COLOR_FORMAT},
          .depth_format = DEPTH_FORMAT,
          .shaders = shader_create_infos,
      };

  pipeline_ = new blu::core::rendering::Pipeline(
      device_, opaque_render_stage_create_info);
}

OpaqueRenderStage::~OpaqueRenderStage() { delete pipeline_; }

void OpaqueRenderStage::Run(VkCommandBuffer buf, uint32_t frame_index,
                            Image* color, Image* depth, uint32_t width,
                            uint32_t height, Buffer* draw_command_buffer,
                            eastl::vector<VkDescriptorSet> descriptor_sets,
                            Buffer* vertex_buffer, Buffer* normal_buffer,
                            Buffer* uv_buffer, Buffer* index_buffer) {
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
      buf, color->image, VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, range);

  blu::core::Image::ImageLayoutTransition(
      buf, depth->image, VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, depth_range);

  eastl::array<VkClearValue, 2> clear_values{};
  clear_values[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
  clear_values[1].depthStencil = {1.0f, 0};

  VkRenderingAttachmentInfo color_attachment_info{
      .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
      .imageView = color->view,
      .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      .resolveMode = VK_RESOLVE_MODE_NONE,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
      .clearValue = clear_values[0],
  };

  VkRenderingAttachmentInfo depth_attachment_info{
      .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
      .imageView = depth->view,
      .imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
      .resolveMode = VK_RESOLVE_MODE_NONE,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
      .clearValue = clear_values[1],
  };
  VkRenderingInfo render_info{
      .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
      .renderArea = {{.x = 0, .y = 0}, width, height},
      .layerCount = 1,
      .viewMask = 0,
      .colorAttachmentCount = 1,
      .pColorAttachments = &color_attachment_info,
      .pDepthAttachment = &depth_attachment_info,
      //.pStencilAttachment = &depth_attachment_info,
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

  vkCmdBindVertexBuffers(buf, 0, 1, &vertex_buffer->buffer, offsets);
  vkCmdBindVertexBuffers(buf, 1, 1, &normal_buffer->buffer, offsets);
  vkCmdBindVertexBuffers(buf, 2, 1, &uv_buffer->buffer, offsets);

  vkCmdBindIndexBuffer(buf, index_buffer->buffer, 0, VK_INDEX_TYPE_UINT32);

  vkCmdDrawIndexedIndirectCount(buf, draw_command_buffer->buffer,
                                sizeof(uint32_t), draw_command_buffer->buffer,
                                0, MAX_MODELS, DRAW_COMMAND_BUFFER_SIZE);

  vkCmdEndRendering(buf);
}
}  // namespace blu::core::rendering