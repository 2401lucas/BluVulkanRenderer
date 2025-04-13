#ifndef ANTIALIASINGSTAGE_H
#define ANTIALIASINGSTAGE_H

#include "../Vulkan/Stage.h"

namespace blu::core::rendering {
class AntiAliasingStage : protected Stage {
 public:
  AntiAliasingStage(Device* device, VmaAllocator allocator,
                    blu::core::rendering::Pipeline* pipeline,
                    eastl::vector<VkCommandPool>& pools, uint32_t width,
                    uint32_t height);
  ~AntiAliasingStage();

  void Resize(uint32_t frame_count, uint32_t width, uint32_t height);

  void Run(uint32_t frame_index, VkSemaphore wait_semaphore,
           uint64_t wait_value, VkPipelineStageFlags wait_flag,
           VkSemaphore signal_semaphore, uint64_t signal_value, VkFence fence);

  eastl::vector<blu::core::Image*> aliased_output_images;

 private:
  uint32_t width_;
  uint32_t height_;
};
}  // namespace blu::core::rendering
#endif