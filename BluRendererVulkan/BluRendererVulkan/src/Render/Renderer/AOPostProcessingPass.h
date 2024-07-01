#pragma once

#include "PostProcessingPass.h"

namespace vks {
class AOPostProcessingPass : public PostProcessingPass {
 public:
  struct {
    VkImage image;
    VkDeviceMemory memory;
    VkImageView view;
    VkDescriptorImageInfo descriptor;
  } randomTex;

  std::vector<float> mSSAOKernel;

  AOPostProcessingPass(VulkanDevice* device, VkFormat colorFormat,
                                 VkFormat depthFormat,
             uint32_t imageCount, float width, float height,
             std::string vertexShaderPath, std::string fragmentShaderPath, VkQueue copyQueue);
  ~AOPostProcessingPass() override;

  void onRender(VkCommandBuffer curBuf, uint32_t currentFrameIndex) override;
  void updateDescriptorSets() override;
  void createUbo() override;
  void updateUbo() override;
};
}  // namespace vks