#ifndef DEPTHONLYSTAGE_H
#define DEPTHONLYSTAGE_H

#include "../Vulkan/Buffer.h"
#include "../Vulkan/Image.h"
#include "../Vulkan/Pipeline.h"
#include "../Vulkan/Stage.h"

namespace blu::core::rendering::stage {
class DepthOnlyStage : protected Stage {
 public:
  DepthOnlyStage(Device* device,
                 eastl::vector<VkDescriptorSetLayout> descriptor_set_layouts,
                 VkPipelineShaderStageCreateInfo);
  ~DepthOnlyStage();

  void Run(VkCommandBuffer buf, blu::core::Image* image, uint32_t width,
           uint32_t height, blu::core::Buffer* draw_command_buffer,
           eastl::vector<VkDescriptorSet> descriptor_sets,
           blu::core::Buffer* vertex_buffer, blu::core::Buffer* index_buffer);

 private:
  blu::core::rendering::Pipeline* pipeline_;
};
}  // namespace blu::core::rendering
#endif