#pragma once
#include "../VulkanResources/VulkanDevice.h"
#include "VulkanglTFModel.h"

namespace vkglTF {
class Scene {
 public:
  vks::VulkanDevice* device;

  void loadFromFile(std::string filename, vks::VulkanDevice* device,
                    VkQueue transferQueue,
                    uint32_t fileLoadingFlags = vkglTF::FileLoadingFlags::None,
                    float scale = 1.0f);
};

}  // namespace vkglTF