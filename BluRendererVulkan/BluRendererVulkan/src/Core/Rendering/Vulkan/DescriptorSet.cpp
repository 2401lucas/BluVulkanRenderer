#include "DescriptorSet.h"

void blu::core::DescriptorSet::Destroy(const VkDevice& device) {
  vkDestroyDescriptorSetLayout(device, layout, nullptr);
}