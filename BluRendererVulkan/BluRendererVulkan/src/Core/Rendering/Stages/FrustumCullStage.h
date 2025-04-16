#ifndef FRUSTUMCULLSTAGE_H
#define FRUSTUMCULLSTAGE_H

#include "../Vulkan/Buffer.h"
#include "../Vulkan/Pipeline.h"
#include "../Vulkan/Stage.h"

namespace blu::core::rendering {
class FrustumCullStage : protected Stage {
 public:
  struct FrustumPushConst {
    BufferInfo model_data;
    BufferInfo models;
    BufferInfo culled_output;
    uint32_t model_count;
  };

  FrustumCullStage(Device* device,
                   VkPipelineShaderStageCreateInfo shader_infos);
  ~FrustumCullStage();

  void Run(VkCommandBuffer buf, uint32_t frame_index, BufferInfo model_data,
           BufferInfo models, uint32_t model_count, Buffer* output_buffer);

  blu::core::rendering::Pipeline* pipeline_;
  eastl::vector<blu::core::Buffer*> frustum_output_buffers_;
};
}  // namespace blu::core::rendering
#endif