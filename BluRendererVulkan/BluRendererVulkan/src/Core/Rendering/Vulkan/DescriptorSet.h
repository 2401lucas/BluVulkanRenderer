#ifndef DESCRIPTORSET_H
#define DESCRIPTORSET_H

#include <vulkan/vulkan_core.h>

namespace blu::core {
class DescriptorSet {
 public:
  VkDescriptorSetLayout layout;
  VkDescriptorSet set;

  void Destroy(const VkDevice& device);
};
}  // namespace blu::core
#endif