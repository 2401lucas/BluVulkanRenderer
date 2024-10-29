#ifndef VULKANINSTANCE_H
#define VULKANINSTANCE_H
#include <EASTL/hash_set.h>
#include <EASTL/string.h>
#include <EASTL/vector.h>
#include <vulkan/vulkan.h>

namespace vk::core {
class Instance {
 public:
  Instance(eastl::string name, bool use_validation,
           eastl::vector<eastl::string> requested_instance_extensions);
  ~Instance();

  VkInstance get() const { return instance_; }

 private:
  uint32_t api_version_ = VK_API_VERSION_1_2;
  VkInstance instance_;
  eastl::hash_set<size_t> supported_instance_extensions_;
  eastl::vector<char*> enabled_instance_extensions_;
};
}  // namespace vk::core

#endif