#ifndef FRUSTUMCULLSTAGE_H
#define FRUSTUMCULLSTAGE_H

#include "../Vulkan/Buffer.h"
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

  FrustumCullStage(Device* device, VmaAllocator allocator,
                   blu::core::rendering::Pipeline* pipeline,
                   eastl::vector<VkCommandPool>& pools);
  ~FrustumCullStage();

  void Run(uint32_t frame_index, BufferInfo model_data, BufferInfo models,
           uint32_t model_count, VkSemaphore wait_semaphore,
           uint64_t wait_value, VkPipelineStageFlags wait_flag,
           VkSemaphore signal_semaphore, uint64_t signal_value, VkFence fence);

  eastl::vector<blu::core::Buffer*> frustum_output_buffers_;
};
}  // namespace blu::core::rendering
#endif