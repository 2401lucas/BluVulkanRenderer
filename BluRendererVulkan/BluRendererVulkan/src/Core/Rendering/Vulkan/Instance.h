#ifndef VULKANINSTANCE_H
#define VULKANINSTANCE_H

#define VK_USE_PLATFORM_WIN32_KHR
#include <EASTL/hash_set.h>
#include <EASTL/string.h>
#include <EASTL/vector.h>
#include <vulkan/vulkan.h>

namespace blu::core {
class Instance {
 public:
  Instance(const eastl::string name, const bool use_validation,
           eastl::vector<eastl::string> requested_instance_extensions);
  ~Instance();

  VkInstance Get() const { return instance_; }
  uint32_t GetApiVersion() const { return api_version_; }

 private:
  uint32_t api_version_ = VK_API_VERSION_1_2;
  VkInstance instance_;
  eastl::hash_set<size_t> supported_extensions_;
  eastl::vector<char*> enabled_instance_extensions_;
};
}  // namespace blu::core

#endif