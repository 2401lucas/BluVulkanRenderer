#ifndef STAGE_H
#define STAGE_H

#include <EASTL/vector.h>
#include <vulkan/vulkan.h>

#include "../ForwardRendererConsts.h"
#include "../Vulkan/Buffer.h"
#include "../Vulkan/Image.h"
#include "../Vulkan/Pipeline.h"
#include "../Vulkan/Tools.h"

namespace blu::core::rendering {
struct SemaphoreData {
  VkSemaphore semaphore;
};

class Stage {
 protected:
  Stage(Device* device, VmaAllocator allocator,
        blu::core::rendering::Pipeline* pipeline,
        eastl::vector<VkCommandPool>& pools);
  ~Stage();
  VkCommandBuffer Begin(uint32_t index);
  void End(uint32_t index);

  VkSubmitInfo PrepareSubmitInfo(VkSemaphore wait_semaphore,
                                 uint64_t wait_value,
                                 VkPipelineStageFlags wait_flag,
                                 VkSemaphore signal_semaphore,
                                 uint64_t signal_value);

  Device* device_;
  VmaAllocator allocator_;

  eastl::vector<VkCommandBuffer> command_buffers;
  blu::core::rendering::Pipeline* pipeline_;

  // TODO FIX : This is a hack, I need this to persist after the function is
  // called, but only until VkQueueSubmit is called. I opted for this hack
  // instead of rewriting a ton of code every time.
  VkTimelineSemaphoreSubmitInfo* timeline_semaphore_values;
};
}  // namespace blu::core::rendering
#endif