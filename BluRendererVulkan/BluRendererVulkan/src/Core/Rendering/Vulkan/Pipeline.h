#ifndef PIPELINE_H
#define PIPELINE_H

#include <EASTL/vector.h>
#include <vulkan/vulkan.h>

#include "Device.h"

namespace blu::core::rendering {

struct GraphicsPipelineCreateInfo {
  eastl::vector<VkDescriptorSetLayout> descriptor_set_layouts;

  VkPipelineInputAssemblyStateCreateFlags input_assembly_flags;
  VkPrimitiveTopology input_assembly_topology;
  VkBool32 input_assembly_primitive_restart_enable;

  VkPipelineRasterizationStateCreateFlags rasteriazation_flags;
  VkPolygonMode rasteriazation_state_polygone_mode;
  VkCullModeFlags rasteriazation_state_cull_mode;
  VkFrontFace rasteriazation_state_front_face;

  eastl::vector<VkPipelineColorBlendAttachmentState>
      color_blend_attachment_states;

  VkBool32 depth_stencil_depth_test;
  VkBool32 depth_stencil_depth_write;
  VkCompareOp depth_stencil_depth_compare_op;
  VkCompareOp depth_stencil_front_compare_op;
  VkCompareOp depth_stencil_back_compare_op;

  VkPipelineViewportStateCreateFlags viewport_flags;
  uint32_t viewport_count;
  uint32_t scissor_count;

  VkPipelineMultisampleStateCreateFlags multisample_flags;
  VkSampleCountFlagBits multisample_count;

  VkPipelineDynamicStateCreateFlags dynamic_state_flags;
  eastl::vector<VkDynamicState> dynamic_state_enables;

  eastl::vector<VkVertexInputBindingDescription> vertex_input_bindings;
  eastl::vector<VkVertexInputAttributeDescription> vertex_input_attributes;

  eastl::vector<VkFormat> color_attachment_formats;
  VkFormat depth_format;
  VkFormat stencil_depth_format;

  eastl::vector<VkPipelineShaderStageCreateInfo> shaders;
};

struct ComputePipelineCreateInfo {};

class Pipeline {
 public:
  Pipeline(Device*, const GraphicsPipelineCreateInfo& pipeline_create_info);
  // Pipeline(Device*, const ComputePipelineCreateInfo& pipeline_create_info);

  ~Pipeline();

  VkPipeline* GetPipeline() { return &pipeline_; };
  VkPipelineLayout* GetPipelineLayout() { return &layout_; };

 private:
  Device* device_;
  VkPipelineLayout layout_;
  VkPipeline pipeline_;
};
}  // namespace blu::core::rendering
#endif