#ifndef OPAQUERENDERSTAGE_H
#define OPAQUERENDERSTAGE_H

#include "../Vulkan/Stage.h"

namespace blu::core::rendering {
class OpaqueRenderStage : protected Stage {
 public:
  OpaqueRenderStage(Device* device, VmaAllocator allocator,
                 blu::core::rendering::Pipeline* pipeline,
                    eastl::vector<VkCommandPool>& pools, uint32_t width,
                    uint32_t height);
  ~OpaqueRenderStage();

  void Resize(uint32_t frame_count, uint32_t width, uint32_t height);

  void Run(uint32_t frame_index, Buffer* draw_command_buffer,
           eastl::vector<VkDescriptorSet> descriptor_sets,
           Buffer* vertex_buffer, Buffer* normal_buffer, Buffer* uv_buffer,
           Buffer* index_buffer,
           VkSemaphore wait_semaphore, uint64_t wait_value,
           VkPipelineStageFlags wait_flag, VkSemaphore signal_semaphore,
           uint64_t signal_value, VkFence fence);

  eastl::vector<blu::core::Image*> color_output_images;

 private:
  uint32_t width_;
  uint32_t height_;

  blu::core::Image* depth_stencil_image_ = nullptr;
};
}  // namespace blu::core::rendering
#endif