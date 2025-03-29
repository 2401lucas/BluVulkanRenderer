#include "Device.h"

#include "Tools.h"

namespace blu::core {
Device::Device(const blu::core::Instance* instance,
               VkPhysicalDeviceFeatures physical_device_requested_features_,
               const eastl::vector<const char*>& requested_features,
               void* p_next) {
  physical_device_enabled_features_ = physical_device_requested_features_;
  uint32_t gpu_count;

  VK_CHECK_RESULT(
      vkEnumeratePhysicalDevices(instance->Get(), &gpu_count, nullptr));

  if (gpu_count == 0) {
    std::cerr << "No rendering devices found";
    exit(-1);
  }

  eastl::vector<VkPhysicalDevice> physical_devices(gpu_count);
  VK_CHECK_RESULT(vkEnumeratePhysicalDevices(instance->Get(), &gpu_count,
                                             physical_devices.data()));

  for (eastl::vector<VkPhysicalDevice>::iterator
           it = physical_devices.begin(),
           it_end = physical_devices.end();
       it != it_end; ++it) {
    vkGetPhysicalDeviceProperties(*it, &physical_device_properties_);
    vkGetPhysicalDeviceFeatures(*it, &physical_device_features_);
    if (physical_device_properties_.deviceType ==
        VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
      physical_device_ = *it;
      break;
    }
  }

  if (!physical_device_) {
    physical_device_ = physical_devices[0];
  }

  vkGetPhysicalDeviceProperties(physical_device_, &physical_device_properties_);
  vkGetPhysicalDeviceFeatures(physical_device_, &physical_device_features_);

  vkGetPhysicalDeviceMemoryProperties(physical_device_,
                                      &physical_device_memory_properties_);

  uint32_t queue_family_count;

  vkGetPhysicalDeviceQueueFamilyProperties(physical_device_,
                                           &queue_family_count, nullptr);
  if (queue_family_count == 0) {
    std::cerr << "No queue families found";
    exit(-1);
  }
  queue_family_properties_.resize(queue_family_count);

  vkGetPhysicalDeviceQueueFamilyProperties(
      physical_device_, &queue_family_count, queue_family_properties_.data());

  uint32_t extension_count = 0;
  vkEnumerateDeviceExtensionProperties(physical_device_, nullptr,
                                       &extension_count, nullptr);
  if (extension_count > 0) {
    eastl::vector<VkExtensionProperties> extensions(extension_count);
    VK_CHECK_RESULT(vkEnumerateDeviceExtensionProperties(
        physical_device_, nullptr, &extension_count, extensions.data()));
    for (eastl::vector<VkExtensionProperties>::iterator
             it = extensions.begin(),
             it_end = extensions.end();
         it != it_end; ++it) {
      supported_extensions_.insert(
          eastl::hash<eastl::string>()(it->extensionName));
    }
  }

  eastl::vector<VkDeviceQueueCreateInfo> queue_create_infos;

  const float default_queue_priority(0.0f);

  queue_family_indicies_.graphics = GetQueueFamilyIndex(VK_QUEUE_GRAPHICS_BIT);
  {
    VkDeviceQueueCreateInfo queue_info{
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = queue_family_indicies_.graphics,
        .queueCount = 1,
        .pQueuePriorities = &default_queue_priority,
    };
    queue_create_infos.push_back(queue_info);
  }

  queue_family_indicies_.compute = GetQueueFamilyIndex(VK_QUEUE_COMPUTE_BIT);
  if (queue_family_indicies_.compute != queue_family_indicies_.graphics) {
    // If compute family index differs, we need an additional queue create
    // info for the compute queue
    VkDeviceQueueCreateInfo queue_info{
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = queue_family_indicies_.compute,
        .queueCount = 1,
        .pQueuePriorities = &default_queue_priority,
    };
    queue_create_infos.push_back(queue_info);
  }

  queue_family_indicies_.transfer = GetQueueFamilyIndex(VK_QUEUE_TRANSFER_BIT);
  if ((queue_family_indicies_.transfer != queue_family_indicies_.graphics) &&
      (queue_family_indicies_.transfer != queue_family_indicies_.compute)) {
    // If transfer family index differs, we need an additional queue create
    // info for the transfer queue
    VkDeviceQueueCreateInfo queueInfo{
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = queue_family_indicies_.transfer,
        .queueCount = 1,
        .pQueuePriorities = &default_queue_priority,
    };
    queue_create_infos.push_back(queueInfo);
  }

  eastl::vector<const char*> device_extensions(requested_features);
  device_extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

  VkDeviceCreateInfo device_ci{
      .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
      .queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size()),
      .pQueueCreateInfos = queue_create_infos.data(),
      .pEnabledFeatures = &physical_device_enabled_features_,
  };

  VkPhysicalDeviceFeatures2 physical_device_features2;
  if (p_next) {
    physical_device_features2 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .pNext = p_next,
        .features = physical_device_enabled_features_,
    };
    device_ci.pEnabledFeatures = nullptr;
    device_ci.pNext = &physical_device_features2;
  }

  if (!device_extensions.empty()) {
    eastl::hash_set<size_t>::iterator supported_extensions_it_end =
        supported_extensions_.end();
    for (eastl::vector<const char*>::iterator it = device_extensions.begin(),
                                              it_end = device_extensions.end();
         it != it_end; ++it) {
      if (supported_extensions_.find(eastl::hash<eastl::string>()(*it)) ==
          supported_extensions_it_end) {
        std::cerr << "Enabled device extension \"" << *it
                  << "\" is not present at device level\n";
        exit(-1);
      }
    }

    device_ci.enabledExtensionCount = device_extensions.size();
    device_ci.ppEnabledExtensionNames = device_extensions.data();
  }

  VK_CHECK_RESULT(
      vkCreateDevice(physical_device_, &device_ci, nullptr, &device_));

  vkGetDeviceQueue(device_, queue_family_indicies_.graphics, 0,
                   &queues.graphics);
  vkGetDeviceQueue(device_, queue_family_indicies_.compute, 0, &queues.compute);
  vkGetDeviceQueue(device_, queue_family_indicies_.transfer, 0,
                   &queues.transfer);

  VmaAllocatorCreateInfo vmaAllocInfo{
      .flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
      .physicalDevice = physical_device_,
      .device = device_,
      .instance = instance->Get(),
      .vulkanApiVersion = instance->GetApiVersion(),
  };

  vmaCreateAllocator(&vmaAllocInfo, &allocator_);
}

Device::~Device() {
  if (allocator_) {
    vmaDestroyAllocator(allocator_);
  }
  if (device_) {
    vkDestroyDevice(device_, nullptr);
  }
}

uint32_t Device::GetQueueFamilyIndex(VkQueueFlags queue_flags) const {
  // Dedicated queue for compute
  // Try to find a queue family index that supports compute but not graphics
  if ((queue_flags & VK_QUEUE_COMPUTE_BIT) == queue_flags) {
    for (uint32_t i = 0;
         i < static_cast<uint32_t>(queue_family_properties_.size()); i++) {
      if ((queue_family_properties_[i].queueFlags & VK_QUEUE_COMPUTE_BIT) &&
          ((queue_family_properties_[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) ==
           0)) {
        return i;
      }
    }
  }

  // Dedicated queue for transfer
  // Try to find a queue family index that supports transfer but not graphics
  // and compute
  if ((queue_flags & VK_QUEUE_TRANSFER_BIT) == queue_flags) {
    for (uint32_t i = 0;
         i < static_cast<uint32_t>(queue_family_properties_.size()); i++) {
      if ((queue_family_properties_[i].queueFlags & VK_QUEUE_TRANSFER_BIT) &&
          ((queue_family_properties_[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) ==
           0) &&
          ((queue_family_properties_[i].queueFlags & VK_QUEUE_COMPUTE_BIT) ==
           0)) {
        return i;
      }
    }
  }

  // For other queue types or if no separate compute queue is present, return
  // the first one to support the requested flags
  for (uint32_t i = 0;
       i < static_cast<uint32_t>(queue_family_properties_.size()); i++) {
    if ((queue_family_properties_[i].queueFlags & queue_flags) == queue_flags) {
      return i;
    }
  }

  throw std::runtime_error("Could not find a matching queue family index");
}
}  // namespace blu::core