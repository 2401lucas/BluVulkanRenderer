#ifndef DESCRIPTORSET_H
#define DESCRIPTORSET_H

#include <vulkan/vulkan_core.h>

namespace blu::core {
struct DescriptorSet {
 public:
  VkDescriptorSetLayout layout;
  VkDescriptorSet set;
};
}  // namespace blu::core
#endif