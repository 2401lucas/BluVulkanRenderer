#include "Pipeline.h"

#include "Tools.h"

namespace blu::core::rendering {

Pipeline::Pipeline(Device* device,
                   const GraphicsPipelineCreateInfo& pipeline_create_info) {
  device_ = device;
  VkPipelineLayoutCreateInfo pipeline_layout_create{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .setLayoutCount = static_cast<uint32_t>(
          pipeline_create_info.descriptor_set_layouts.size()),
      .pSetLayouts = pipeline_create_info.descriptor_set_layouts.data(),
  };

  vkCreatePipelineLayout(device->GetLogicalDevice(), &pipeline_layout_create,
                         nullptr, &layout_);

  VkPipelineInputAssemblyStateCreateInfo input_assembly_state{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
      .flags = pipeline_create_info.input_assembly_flags,
      .topology = pipeline_create_info.input_assembly_topology,
      .primitiveRestartEnable =
          pipeline_create_info.input_assembly_primitive_restart_enable,
  };

  VkPipelineRasterizationStateCreateInfo rasterization_state{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
      .flags = pipeline_create_info.rasteriazation_flags,
      .polygonMode = pipeline_create_info.rasteriazation_state_polygone_mode,
      .cullMode = pipeline_create_info.rasteriazation_state_cull_mode,
      .frontFace = pipeline_create_info.rasteriazation_state_front_face,
  };

  VkPipelineColorBlendStateCreateInfo color_blend_state{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
      .attachmentCount = static_cast<uint32_t>(
          pipeline_create_info.color_blend_attachment_states.size()),
      .pAttachments = pipeline_create_info.color_blend_attachment_states.data(),
  };

  VkPipelineDepthStencilStateCreateInfo depth_stencil_state{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
      .depthTestEnable = pipeline_create_info.depth_stencil_depth_test,
      .depthWriteEnable = pipeline_create_info.depth_stencil_depth_write,
      .depthCompareOp = pipeline_create_info.depth_stencil_depth_compare_op,
      .front{
          .compareOp = pipeline_create_info.depth_stencil_front_compare_op,
      },
      .back{
          .compareOp = pipeline_create_info.depth_stencil_back_compare_op,
      },
  };

  VkPipelineViewportStateCreateInfo viewport_state{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
      .flags = pipeline_create_info.viewport_flags,
      .viewportCount = pipeline_create_info.viewport_count,
      .scissorCount = pipeline_create_info.scissor_count,
  };

  VkPipelineMultisampleStateCreateInfo multisample_state{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
      .flags = pipeline_create_info.multisample_flags,
      .rasterizationSamples = pipeline_create_info.multisample_count,
  };

  VkPipelineDynamicStateCreateInfo dynamic_state{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
      .flags = pipeline_create_info.dynamic_state_flags,
      .dynamicStateCount = static_cast<uint32_t>(
          pipeline_create_info.dynamic_state_enables.size()),
      .pDynamicStates = pipeline_create_info.dynamic_state_enables.data(),
  };

  VkPipelineVertexInputStateCreateInfo vertex_input_state{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
      .vertexBindingDescriptionCount = static_cast<uint32_t>(
          pipeline_create_info.vertex_input_bindings.size()),
      .pVertexBindingDescriptions =
          static_cast<uint32_t>(
              pipeline_create_info.vertex_input_bindings.size()) != 0
              ? pipeline_create_info.vertex_input_bindings.data()
              : nullptr,
      .vertexAttributeDescriptionCount = static_cast<uint32_t>(
          pipeline_create_info.vertex_input_attributes.size()),
      .pVertexAttributeDescriptions =
          static_cast<uint32_t>(
              pipeline_create_info.vertex_input_attributes.size()) != 0
              ? pipeline_create_info.vertex_input_attributes.data()
              : nullptr,
  };

  VkPipelineRenderingCreateInfo pipeline_create{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
      .colorAttachmentCount = static_cast<uint32_t>(
          pipeline_create_info.color_attachment_formats.size()),
      .pColorAttachmentFormats =
          pipeline_create_info.color_attachment_formats.data(),
      .depthAttachmentFormat = pipeline_create_info.depth_format,
      .stencilAttachmentFormat = pipeline_create_info.stencil_depth_format,
  };

  VkGraphicsPipelineCreateInfo graphics_create{
      .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
      .pNext = &pipeline_create,
      .stageCount = static_cast<uint32_t>(pipeline_create_info.shaders.size()),
      .pStages = pipeline_create_info.shaders.data(),
      .pVertexInputState = &vertex_input_state,
      .pInputAssemblyState = &input_assembly_state,
      .pViewportState = &viewport_state,
      .pRasterizationState = &rasterization_state,
      .pMultisampleState = &multisample_state,
      .pDepthStencilState = &depth_stencil_state,
      .pColorBlendState = &color_blend_state,
      .pDynamicState = &dynamic_state,
      .layout = layout_,
      .renderPass = VK_NULL_HANDLE,
  };

  VK_CHECK_RESULT(vkCreateGraphicsPipelines(device->GetLogicalDevice(), nullptr,
                                            1, &graphics_create, nullptr,
                                            &pipeline_));
}

Pipeline::~Pipeline() {
  vkDestroyPipelineLayout(device_->GetLogicalDevice(), layout_, nullptr);
  vkDestroyPipeline(device_->GetLogicalDevice(), pipeline_, nullptr);
}
}  // namespace blu::core::rendering