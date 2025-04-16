#ifndef BUILDCOMMANDBUFFERSTAGE_H
#define BUILDCOMMANDBUFFERSTAGE_H

#include "../Vulkan/Buffer.h"
#include "../Vulkan/Image.h"
#include "../Vulkan/Pipeline.h"
#include "../Vulkan/Stage.h"

namespace blu::core::rendering {
class BuildCommandBufferStage : protected Stage {
 public:
  struct BuildCommandBufferPushConst {
    BufferInfo model_data;
    BufferInfo models;
    BufferInfo culled_output;
    uint32_t model_count;
  };

  BuildCommandBufferStage(Device* device,
                          VkPipelineShaderStageCreateInfo shader_create_info);
  ~BuildCommandBufferStage();

  void Run(VkCommandBuffer buf, uint32_t frame_index, BufferInfo model_data,
           BufferInfo models, uint32_t model_count, Buffer* output_buffer);

  blu::core::rendering::Pipeline* pipeline_;
};
}  // namespace blu::core::rendering
#endif