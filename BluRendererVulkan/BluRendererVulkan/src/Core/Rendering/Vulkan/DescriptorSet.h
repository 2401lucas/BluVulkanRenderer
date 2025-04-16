#ifndef DESCRIPTORSET_H
#define DESCRIPTORSET_H

#include <EASTL/vector.h>
#include <vulkan/vulkan_core.h>

namespace blu::core {
class DescriptorSet {
 public:
  VkDescriptorSetLayout layout;
  eastl::vector<VkDescriptorSet> sets;

  void Destroy(const VkDevice& device);
};
}  // namespace blu::core
#endif