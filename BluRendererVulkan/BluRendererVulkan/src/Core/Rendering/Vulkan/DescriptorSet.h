#ifndef DESCRIPTORSET_H
#define DESCRIPTORSET_H

#include <vulkan/vulkan_core.h>

// blu::core gives error qualified name is not allowed
namespace blu {
namespace core {

struct DescriptorSet {
 public:
  VkDescriptorSetLayout layout;
  VkDescriptorSet set;
};
}  // namespace core
}  // namespace blu
#endif