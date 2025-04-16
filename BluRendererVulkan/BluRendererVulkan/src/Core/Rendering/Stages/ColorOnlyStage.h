#ifndef COLORONLYSTAGE_H
#define COLORONLYSTAGE_H

#include "../Vulkan/Image.h"
#include "../Vulkan/Pipeline.h"
#include "../Vulkan/Stage.h"

namespace blu::core::rendering {
class ColorOnlyStage : protected Stage {
 public:
  ColorOnlyStage(Device* device,
                 eastl::vector<VkDescriptorSetLayout> descriptor_set_layouts,
                 eastl::vector<VkPipelineShaderStageCreateInfo> shader_infos);
  ~ColorOnlyStage();

  void Run(VkCommandBuffer buf, eastl::vector<VkDescriptorSet> descriptor_sets,
           Image* output, uint32_t width, uint32_t height);

 private:
  blu::core::rendering::Pipeline* pipeline_;
};
}  // namespace blu::core::rendering
#endif