#ifndef OPAQUERENDERSTAGE_H
#define OPAQUERENDERSTAGE_H

#include "../Vulkan/Buffer.h"
#include "../Vulkan/Image.h"
#include "../Vulkan/Pipeline.h"
#include "../Vulkan/Stage.h"

namespace blu::core::rendering {
class OpaqueRenderStage : protected Stage {
 public:
  OpaqueRenderStage(Device* device,
                    eastl::vector<VkDescriptorSetLayout> descriptor_set_layouts,
                    eastl::vector<VkPipelineShaderStageCreateInfo>);
  ~OpaqueRenderStage();

  void Resize(uint32_t frame_count, uint32_t width, uint32_t height);

  void Run(VkCommandBuffer, uint32_t frame_index, Image* color, Image* depth,
           uint32_t width, uint32_t height, Buffer* draw_command_buffer,
           eastl::vector<VkDescriptorSet> descriptor_sets,
           Buffer* vertex_buffer, Buffer* normal_buffer, Buffer* uv_buffer,
           Buffer* index_buffer);

 private:
  blu::core::rendering::Pipeline* pipeline_;
};
}  // namespace blu::core::rendering
#endif