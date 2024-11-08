#include "Instance.h"

#include <EASTL/fixed_vector.h>
#include <EASTL/functional.h>  // For eastl::hash
#include <vulkan/vulkan_win32.h>

#include "Tools.h"

namespace blu::core {
Instance::Instance(const eastl::string name, const bool use_validation,
                   eastl::vector<eastl::string> requested_instance_extensions) {
  // TODO:: CHECK THIS
  // EA::EASTL::Allocator::Init();

  VkApplicationInfo app_info = {
      .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
      .pApplicationName = name.data(),
      .pEngineName = "Blu Renderer: Vulkan",
      .apiVersion = api_version_,
  };

  eastl::fixed_vector<const char *, 2> required_instance_extensions = {
      VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME};

  uint32_t extCount = 0;
  vkEnumerateInstanceExtensionProperties(nullptr, &extCount, nullptr);

  if (extCount > 0) {
    eastl::vector<VkExtensionProperties> extensions(extCount);
    VK_CHECK_RESULT(vkEnumerateInstanceExtensionProperties(
        nullptr, &extCount, &extensions.front()));
    for (eastl::vector<VkExtensionProperties>::iterator
             it = extensions.begin(),
             it_end = extensions.end();
         it != it_end; ++it) {
      supported_extensions_.insert(
          eastl::hash<eastl::string>()(it->extensionName));
    }
  }

  enabled_instance_extensions_.reserve(required_instance_extensions.size() +
                                       requested_instance_extensions.size());
  if (!requested_instance_extensions.empty()) {
    auto supported_instance_it_end = supported_extensions_.end();
    for (eastl::vector<eastl::string>::iterator
             it = requested_instance_extensions.begin(),
             it_end = requested_instance_extensions.end();
         it != it_end; ++it) {
      if (supported_extensions_.find(eastl::hash<eastl::string>()(*it)) !=
          supported_instance_it_end) {
        EASTL_ASSERT(false);
      }
      enabled_instance_extensions_.push_back(it->data());
    }
  }
  enabled_instance_extensions_.set_capacity();

  VkInstanceCreateInfo instance_ci = {
      .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
      .pNext = NULL,
      .pApplicationInfo = &app_info,
  };

  VkDebugUtilsMessengerCreateInfoEXT debug_utils_Messenger_ci;
  if (use_validation) {
    debug_utils_Messenger_ci = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
    instance_ci.pNext = &debug_utils_Messenger_ci;
  }

  if (!enabled_instance_extensions_.empty()) {
    instance_ci.enabledExtensionCount = enabled_instance_extensions_.size();
    instance_ci.ppEnabledExtensionNames = enabled_instance_extensions_.data();
  }

  if (use_validation) {
    const char *validation_layer_name = "VK_LAYER_KHRONOS_validation";

    uint32_t instance_layer_count;
    vkEnumerateInstanceLayerProperties(&instance_layer_count, nullptr);
    eastl::vector<VkLayerProperties> instance_layer_properties(
        instance_layer_count);
    vkEnumerateInstanceLayerProperties(&instance_layer_count,
                                       instance_layer_properties.data());

    bool validation_layer_present = false;
    for (eastl::vector<VkLayerProperties>::iterator
             it = instance_layer_properties.begin(),
             it_end = instance_layer_properties.end();
         it != it_end; ++it) {
      if (strcmp(it->layerName, validation_layer_name)) {
        validation_layer_present = true;
        break;
      }
    }
    if (validation_layer_present) {
      instance_ci.ppEnabledLayerNames = &validation_layer_name;
      instance_ci.enabledLayerCount = 1;
    } else {
      std::cout << "Validation layer VK_LAYER_KHRONOS_validation not present, "
                   "validation is disabled";
    }
  }

  VK_CHECK_RESULT(vkCreateInstance(&instance_ci, nullptr, &instance_));
}

Instance::~Instance() {
  if (instance_) {
    vkDestroyInstance(instance_, nullptr);
  }
}
}  // namespace blu::core