#ifndef ANTIALIASINGSTAGE_H
#define ANTIALIASINGSTAGE_H

#include "../Vulkan/Image.h"
#include "../Vulkan/Pipeline.h"
#include "../Vulkan/Stage.h"

namespace blu::core::rendering::stage {
class AntiAliasingStage : protected Stage {
 public:
  AntiAliasingStage(Device* device,
                    eastl::vector<VkDescriptorSetLayout> descriptor_set_layouts,
                    VkPipelineShaderStageCreateInfo shader_infos);
  ~AntiAliasingStage();

  void Run(VkCommandBuffer buf, eastl::vector<VkDescriptorSet> descriptor_sets,
           uint32_t width, uint32_t height);

 
 private:
     struct AntiAliasingPushConst {
         uint32_t width;
         uint32_t height;
  };

  blu::core::rendering::Pipeline* pipeline_;
};
}  // namespace blu::core::rendering
#endif