#include "VulkanDevice.h"

#include <bit>
#include <map>
#include <unordered_set>

namespace core_internal::rendering::vulkan {
VulkanDevice::VulkanDevice(const char *name, bool useValidation,
                           std::vector<const char *> enabledDeviceExtensions,
                           std::vector<const char *> enabledInstanceExtensions,
                           void *pNextChain) {
  VkApplicationInfo appInfo = {};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = name;
  appInfo.pEngineName = "Blu: Vulkan";
  appInfo.apiVersion = apiVersion;

  std::vector<const char *> instanceExtensions = {
      VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME};

  // Get extensions supported by the instance and store for later use
  uint32_t extCount = 0;
  vkEnumerateInstanceExtensionProperties(nullptr, &extCount, nullptr);
  if (extCount > 0) {
    std::vector<VkExtensionProperties> extensions(extCount);
    if (vkEnumerateInstanceExtensionProperties(
            nullptr, &extCount, &extensions.front()) == VK_SUCCESS) {
      for (VkExtensionProperties &extension : extensions) {
        supportedInstanceExtensions.push_back(extension.extensionName);
      }
    }
  }

  // Enabled requested instance extensions
  if (enabledInstanceExtensions.size() > 0) {
    for (const char *enabledExtension : enabledInstanceExtensions) {
      // Output message if requested extension is not available
      if (std::find(supportedInstanceExtensions.begin(),
                    supportedInstanceExtensions.end(),
                    enabledExtension) == supportedInstanceExtensions.end()) {
        std::cerr << "Enabled instance extension \"" << enabledExtension
                  << "\" is not present at instance level\n";
      }
      instanceExtensions.push_back(enabledExtension);
    }
  }

  VkInstanceCreateInfo instanceCreateInfo = {};
  instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  instanceCreateInfo.pNext = NULL;
  instanceCreateInfo.pApplicationInfo = &appInfo;

  VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCI{};
  if (useValidation) {
    debugUtilsMessengerCI =
        core_internal::rendering::debug::setupDebugingMessengerCreateInfo();
    debugUtilsMessengerCI.pNext = instanceCreateInfo.pNext;
    instanceCreateInfo.pNext = &debugUtilsMessengerCI;
  }

  // Enable the debug utils extension if available (e.g. when debugging tools
  // are present)
  if (useValidation || std::find(supportedInstanceExtensions.begin(),
                                 supportedInstanceExtensions.end(),
                                 VK_EXT_DEBUG_UTILS_EXTENSION_NAME) !=
                           supportedInstanceExtensions.end()) {
    instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  }

  if (instanceExtensions.size() > 0) {
    instanceCreateInfo.enabledExtensionCount =
        (uint32_t)instanceExtensions.size();
    instanceCreateInfo.ppEnabledExtensionNames = instanceExtensions.data();
  }

  // The VK_LAYER_KHRONOS_validation contains all current validation
  // functionality.
  const char *validationLayerName = "VK_LAYER_KHRONOS_validation";
  if (useValidation) {
    // Check if this layer is available at instance level
    uint32_t instanceLayerCount;
    vkEnumerateInstanceLayerProperties(&instanceLayerCount, nullptr);
    std::vector<VkLayerProperties> instanceLayerProperties(instanceLayerCount);
    vkEnumerateInstanceLayerProperties(&instanceLayerCount,
                                       instanceLayerProperties.data());
    bool validationLayerPresent = false;
    for (VkLayerProperties &layer : instanceLayerProperties) {
      if (strcmp(layer.layerName, validationLayerName) == 0) {
        validationLayerPresent = true;
        break;
      }
    }
    if (validationLayerPresent) {
      instanceCreateInfo.ppEnabledLayerNames = &validationLayerName;
      instanceCreateInfo.enabledLayerCount = 1;
    } else {
      std::cerr << "Validation layer VK_LAYER_KHRONOS_validation not present, "
                   "validation is disabled";
    }
  }

  VK_CHECK_RESULT(vkCreateInstance(&instanceCreateInfo, nullptr, &instance));

  // If the debug utils extension is present we set up debug functions for
  // labeling objects for debugging
  if (std::find(supportedInstanceExtensions.begin(),
                supportedInstanceExtensions.end(),
                VK_EXT_DEBUG_UTILS_EXTENSION_NAME) !=
      supportedInstanceExtensions.end()) {
    core_internal::rendering::debug::setup(instance);
  }

  assert(physicalDevice);

  vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);
  vkGetPhysicalDeviceFeatures(physicalDevice, &physicalDeviceFeatures);
  vkGetPhysicalDeviceMemoryProperties(physicalDevice,
                                      &physicalDeviceMemoryProperties);

  uint32_t queueFamilyCount;
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount,
                                           nullptr);
  assert(queueFamilyCount > 0);
  queueFamilyProperties.resize(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount,
                                           queueFamilyProperties.data());

  // Get list of supported extensions
  uint32_t extCount = 0;
  vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extCount,
                                       nullptr);
  if (extCount > 0) {
    std::vector<VkExtensionProperties> extensions(extCount);
    if (vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extCount,
                                             &extensions.front()) ==
        VK_SUCCESS) {
      for (auto ext : extensions) {
        supportedExtensions.push_back(ext.extensionName);
      }
    }
  }

  // If requested, we enable the default validation layers for debugging
  if (useValidation) {
    core_internal::rendering::debug::setupDebugging(instance);
  }

  // Physical device
  uint32_t gpuCount = 0;
  // Get number of available physical devices
  VK_CHECK_RESULT(vkEnumeratePhysicalDevices(instance, &gpuCount, nullptr));
  if (gpuCount == 0) {
    core_internal::rendering::tools::exitFatal(
        "No device with Vulkan support found", -1);
    return;
  }

  // Enumerate devices
  std::vector<VkPhysicalDevice> physicalDevices(gpuCount);
  VK_CHECK_RESULT(
      vkEnumeratePhysicalDevices(instance, &gpuCount, physicalDevices.data()));

  // GPU selection
  physicalDevice = choosePhysicalDevice(physicalDevices);

  // Desired queues need to be requested upon logical device creation
  // Due to differing queue family configurations of Vulkan implementations this
  // can be a bit tricky, especially if the application requests different queue
  // types
  std::vector<VkDeviceQueueCreateInfo> queueCreateInfos{};

  // Get queue family indices for the requested queue family types
  // Note that the indices may overlap depending on the implementation

  const float defaultQueuePriority(0.0f);

  queueFamilyIndices.graphics = getQueueFamilyIndex(VK_QUEUE_GRAPHICS_BIT);
  VkDeviceQueueCreateInfo queueInfo{};
  queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  queueInfo.queueFamilyIndex = queueFamilyIndices.graphics;
  queueInfo.queueCount = 1;
  queueInfo.pQueuePriorities = &defaultQueuePriority;
  queueCreateInfos.push_back(queueInfo);

  queueFamilyIndices.compute = getQueueFamilyIndex(VK_QUEUE_COMPUTE_BIT);
  if (queueFamilyIndices.compute != queueFamilyIndices.graphics) {
    // If compute family index differs, we need an additional queue create
    // info for the compute queue
    VkDeviceQueueCreateInfo queueInfo{};
    queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueInfo.queueFamilyIndex = queueFamilyIndices.compute;
    queueInfo.queueCount = 1;
    queueInfo.pQueuePriorities = &defaultQueuePriority;
    queueCreateInfos.push_back(queueInfo);
  }

  queueFamilyIndices.transfer = getQueueFamilyIndex(VK_QUEUE_TRANSFER_BIT);
  if ((queueFamilyIndices.transfer != queueFamilyIndices.graphics) &&
      (queueFamilyIndices.transfer != queueFamilyIndices.compute)) {
    // If transfer family index differs, we need an additional queue create
    // info for the transfer queue
    VkDeviceQueueCreateInfo queueInfo{};
    queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueInfo.queueFamilyIndex = queueFamilyIndices.transfer;
    queueInfo.queueCount = 1;
    queueInfo.pQueuePriorities = &defaultQueuePriority;
    queueCreateInfos.push_back(queueInfo);
  }

  // Create the logical device representation
  std::vector<const char *> deviceExtensions(enabledDeviceExtensions);
  deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

  VkDeviceCreateInfo deviceCreateInfo = {};
  deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  deviceCreateInfo.queueCreateInfoCount =
      static_cast<uint32_t>(queueCreateInfos.size());
  deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
  deviceCreateInfo.pEnabledFeatures = &physicalDeviceEnabledFeatures;

  VkPhysicalDeviceFeatures2 physicalDeviceFeatures2{};
  if (pNextChain) {
    physicalDeviceFeatures2.sType =
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    physicalDeviceFeatures2.features = physicalDeviceEnabledFeatures;
    physicalDeviceFeatures2.pNext = pNextChain;
    deviceCreateInfo.pEnabledFeatures = nullptr;
    deviceCreateInfo.pNext = &physicalDeviceFeatures2;
  }

  if (deviceExtensions.size() > 0) {
    for (const char *enabledExtension : deviceExtensions) {
      if (!extensionSupported(enabledExtension)) {
        std::cerr << "Enabled device extension \"" << enabledExtension
                  << "\" is not present at device level\n";
      }
    }

    deviceCreateInfo.enabledExtensionCount = (uint32_t)deviceExtensions.size();
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
  }

  VK_CHECK_RESULT(vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr,
                                 &logicalDevice));
  VmaAllocatorCreateInfo vmaAllocInfo{};
  vmaAllocInfo.device = logicalDevice;
  vmaAllocInfo.physicalDevice = physicalDevice;
  vmaAllocInfo.instance = instance;
  vmaAllocInfo.vulkanApiVersion = apiVersion;

  vmaCreateAllocator(&vmaAllocInfo, &allocator);
}

VulkanDevice::~VulkanDevice() {
  for (auto &shaderModule : shaderModules) {
    vkDestroyShaderModule(logicalDevice, shaderModule, nullptr);
  }

  if (allocator) {
    vmaDestroyAllocator(allocator);
  }
  if (logicalDevice) {
    vkDestroyDevice(logicalDevice, nullptr);
  }
  if (instance) {
    vkDestroyInstance(instance, nullptr);
  }
}

VkPhysicalDevice VulkanDevice::choosePhysicalDevice(
    std::vector<VkPhysicalDevice> devices) {
  std::multimap<int, VkPhysicalDevice> candidates;

  for (auto &device : devices) {
    int score = rateDeviceSuitability(device);
    candidates.insert(std::make_pair(score, device));
  }

  if (candidates.rbegin()->first > 0) {
    return candidates.rbegin()->second;
  } else {
    throw std::runtime_error("failed to find a suitable GPU!");
  }
}

int VulkanDevice::rateDeviceSuitability(VkPhysicalDevice device) {
  VkPhysicalDeviceProperties deviceProperties;
  VkPhysicalDeviceFeatures deviceFeatures;
  vkGetPhysicalDeviceProperties(device, &deviceProperties);
  vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

  // Required Features
  /*if (deviceFeatures.geometryShader != VK_TRUE) {
    return 0;
  }*/

  int score = 0;

  if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
    score += 10000;
  }
  // Maximum possible size of textures affects graphics quality
  score += deviceProperties.limits.maxImageDimension2D;

  return score;
}

uint32_t VulkanDevice::getMemoryType(uint32_t typeBits,
                                     VkMemoryPropertyFlags properties,
                                     VkBool32 *memTypeFound) const {
  for (uint32_t i = 0; i < physicalDeviceMemoryProperties.memoryTypeCount;
       i++) {
    if ((typeBits & 1) == 1) {
      if ((physicalDeviceMemoryProperties.memoryTypes[i].propertyFlags &
           properties) == properties) {
        if (memTypeFound) {
          *memTypeFound = true;
        }
        return i;
      }
    }
    typeBits >>= 1;
  }

  if (memTypeFound) {
    *memTypeFound = false;
    return 0;
  } else {
    throw std::runtime_error("Could not find a matching memory type");
  }
}

uint32_t VulkanDevice::getQueueFamilyIndex(VkQueueFlags queueFlags) const {
  // Dedicated queue for compute
  // Try to find a queue family index that supports compute but not graphics
  if ((queueFlags & VK_QUEUE_COMPUTE_BIT) == queueFlags) {
    for (uint32_t i = 0;
         i < static_cast<uint32_t>(queueFamilyProperties.size()); i++) {
      if ((queueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) &&
          ((queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) ==
           0)) {
        return i;
      }
    }
  }

  // Dedicated queue for transfer
  // Try to find a queue family index that supports transfer but not graphics
  // and compute
  if ((queueFlags & VK_QUEUE_TRANSFER_BIT) == queueFlags) {
    for (uint32_t i = 0;
         i < static_cast<uint32_t>(queueFamilyProperties.size()); i++) {
      if ((queueFamilyProperties[i].queueFlags & VK_QUEUE_TRANSFER_BIT) &&
          ((queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) ==
           0) &&
          ((queueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) == 0)) {
        return i;
      }
    }
  }

  // For other queue types or if no separate compute queue is present, return
  // the first one to support the requested flags
  for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilyProperties.size());
       i++) {
    if ((queueFamilyProperties[i].queueFlags & queueFlags) == queueFlags) {
      return i;
    }
  }

  throw std::runtime_error("Could not find a matching queue family index");
}

void VulkanDevice::waitIdle() { vkDeviceWaitIdle(logicalDevice); }

VkPipelineShaderStageCreateInfo VulkanDevice::loadShader(
    std::string fileName, VkShaderStageFlagBits stage) {
  VkPipelineShaderStageCreateInfo shaderStage = {};
  shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shaderStage.stage = stage;
  shaderStage.module = core_internal::rendering::tools::loadShader(
      fileName.c_str(), logicalDevice);
  shaderStage.pName = "main";
  assert(shaderStage.module != VK_NULL_HANDLE);
  shaderModules.push_back(shaderStage.module);
  return shaderStage;
}

bool VulkanDevice::extensionSupported(std::string extension) {
  return (std::find(supportedExtensions.begin(), supportedExtensions.end(),
                    extension) != supportedExtensions.end());
}

VkFormat VulkanDevice::getSupportedDepthFormat(bool checkSamplingSupport) {
  // All depth formats may be optional, so we need to find a suitable depth
  // format to use
  std::vector<VkFormat> depthFormats = {
      VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT,
      VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D16_UNORM_S8_UINT,
      VK_FORMAT_D16_UNORM};
  for (auto &format : depthFormats) {
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(physicalDevice, format,
                                        &formatProperties);
    // Format must support depth stencil attachment for optimal tiling
    if (formatProperties.optimalTilingFeatures &
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
      if (checkSamplingSupport) {
        if (!(formatProperties.optimalTilingFeatures &
              VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT)) {
          continue;
        }
      }
      return format;
    }
  }
  throw std::runtime_error("Could not find a matching depth format");
}

vks::Buffer *VulkanDevice::createBuffer(
    VkBufferUsageFlags usageFlags, VkDeviceSize size,
    VkMemoryPropertyFlags memoryPropertyFlags, uint32_t instanceCount) {
  VkBufferCreateInfo bufferCreateInfo{
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .size = size,
      .usage = usageFlags};

  VmaAllocationCreateInfo allocInfo = {.usage = VMA_MEMORY_USAGE_AUTO};

  VkBuffer buffer;
  VmaAllocation alloc;
  vmaCreateBuffer(allocator, &bufferCreateInfo, &allocInfo, &buffer, &alloc,
                  nullptr);

  auto newBuffer = new vks::Buffer();
  newBuffer->size = size;
  newBuffer->usageFlags = usageFlags;
  newBuffer->memoryPropertyFlags = memoryPropertyFlags;

  newBuffer->buffer.resize(instanceCount);
  newBuffer->memory.resize(instanceCount);
  newBuffer->descriptor.resize(instanceCount);
  for (size_t i = 0; i < instanceCount; i++) {
    VK_CHECK_RESULT(vkCreateBuffer(logicalDevice, &bufferCreateInfo, nullptr,
                                   &newBuffer->buffer[i]));

    VkMemoryRequirements memReqs{};
    VkMemoryAllocateInfo memAlloc{.sType =
                                      VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    vkGetBufferMemoryRequirements(logicalDevice, newBuffer->buffer[i],
                                  &memReqs);
    memAlloc.allocationSize = memReqs.size;
    // Find a memory type index that fits the properties of the buffer
    memAlloc.memoryTypeIndex =
        getMemoryType(memReqs.memoryTypeBits, memoryPropertyFlags);
    // If the buffer has VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT set we also
    // need to enable the appropriate flag during allocation
    VkMemoryAllocateFlagsInfoKHR allocFlagsInfo{};
    if (usageFlags & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) {
      allocFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO_KHR;
      allocFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;
      memAlloc.pNext = &allocFlagsInfo;
    }
    VK_CHECK_RESULT(vkAllocateMemory(logicalDevice, &memAlloc, nullptr,
                                     &newBuffer->memory[i]));
    newBuffer->alignment = memReqs.alignment;

    // newBuffer->setupDescriptor();
  }
}

Image *VulkanDevice::createImage(const ImageInfo &imageCreateInfo,
                                 const VkMemoryPropertyFlagBits &memoryFlags,
                                 bool renderResource) {
  auto newImage = new Image();
  VkImageCreateInfo imgCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      .imageType = VK_IMAGE_TYPE_2D,
      .format = imageCreateInfo.format,
      .extent = {.width = imageCreateInfo.width,
                 .height = imageCreateInfo.height,
                 .depth = imageCreateInfo.depth},
      .mipLevels = imageCreateInfo.mipLevels,
      .arrayLayers = imageCreateInfo.arrayLayers,
      .samples = imageCreateInfo.samples,
      .tiling = imageCreateInfo.tiling,
      .usage = imageCreateInfo.usage,
      .initialLayout = imageCreateInfo.initialLayout,
  };
  // allocCreateInfo.priority = 1.0f; Read more about this to better understand
  // advantages
  VmaAllocationCreateInfo allocCreateInfo = {
      .usage = VMA_MEMORY_USAGE_AUTO,
      .preferredFlags = memoryFlags,
  };

  if (renderResource)
    allocCreateInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
  else if (imageCreateInfo.requireMappedData)
    allocCreateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

  VmaAllocationInfo allocInfo;
  VK_CHECK_RESULT(vmaCreateImage(allocator, &imgCreateInfo, &allocCreateInfo,
                                 &newImage->image, &newImage->deviceMemory,
                                 &allocInfo));

  newImage->mappedData = allocInfo.pMappedData;

  if (imageCreateInfo.requireSampler) {
    VkSamplerCreateInfo samplerCreateInfo{
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = imageCreateInfo.samplerInfo.magFilter,
        .minFilter = imageCreateInfo.samplerInfo.minFilter,
        .mipmapMode = imageCreateInfo.samplerInfo.mipmapMode,
        .addressModeU = imageCreateInfo.samplerInfo.addressModeU,
        .addressModeV = imageCreateInfo.samplerInfo.addressModeV,
        .addressModeW = imageCreateInfo.samplerInfo.addressModeW,
        .mipLodBias = imageCreateInfo.samplerInfo.mipLodBias,
        .maxAnisotropy = imageCreateInfo.samplerInfo.maxAnisotropy,
        .minLod = imageCreateInfo.samplerInfo.minLod,
        .maxLod = imageCreateInfo.samplerInfo.maxLod,
        .borderColor = imageCreateInfo.samplerInfo.borderColor,
    };
    VK_CHECK_RESULT(vkCreateSampler(logicalDevice, &samplerCreateInfo, nullptr,
                                    &newImage->sampler));
  }

  if (imageCreateInfo.requireImageView) {
    VkImageViewCreateInfo imageViewCreateInfo{
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .flags = imageCreateInfo.imageViewInfo.flags,
        .image = newImage->image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = imageCreateInfo.format,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        }};
    VK_CHECK_RESULT(vkCreateImageView(logicalDevice, &imageViewCreateInfo,
                                      nullptr, &newImage->view));
  }

  if (renderResource) {
    newImage->descriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    newImage->descriptor.sampler = newImage->sampler;
    newImage->descriptor.imageView = newImage->view;
  }

  return newImage;
}

std::vector<Image *> VulkanDevice::createAliasedImages(
    std::vector<ImageInfo> imageCreateInfos) {
  std::vector<Image *> newImages;
  std::vector<ResourceReservation> resourceReservations;

  for (auto &imageCreateInfo : imageCreateInfos) {
    auto newImage = new Image();
    VkImageCreateInfo imgCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = imageCreateInfo.format,
        .extent = {.width = imageCreateInfo.width,
                   .height = imageCreateInfo.height,
                   .depth = imageCreateInfo.depth},
        .mipLevels = imageCreateInfo.mipLevels,
        .arrayLayers = imageCreateInfo.arrayLayers,
        .samples = imageCreateInfo.samples,
        .tiling = imageCreateInfo.tiling,
        .usage = imageCreateInfo.usage,
        .initialLayout = imageCreateInfo.initialLayout,
    };

    VK_CHECK_RESULT(vkCreateImage(logicalDevice, &imgCreateInfo, nullptr,
                                  &newImage->image));

    VkSamplerCreateInfo samplerCreateInfo{
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = imageCreateInfo.samplerInfo.magFilter,
        .minFilter = imageCreateInfo.samplerInfo.minFilter,
        .mipmapMode = imageCreateInfo.samplerInfo.mipmapMode,
        .addressModeU = imageCreateInfo.samplerInfo.addressModeU,
        .addressModeV = imageCreateInfo.samplerInfo.addressModeV,
        .addressModeW = imageCreateInfo.samplerInfo.addressModeW,
        .mipLodBias = imageCreateInfo.samplerInfo.mipLodBias,
        .maxAnisotropy = imageCreateInfo.samplerInfo.maxAnisotropy,
        .minLod = imageCreateInfo.samplerInfo.minLod,
        .maxLod = imageCreateInfo.samplerInfo.maxLod,
        .borderColor = imageCreateInfo.samplerInfo.borderColor,
    };
    VK_CHECK_RESULT(vkCreateSampler(logicalDevice, &samplerCreateInfo, nullptr,
                                    &newImage->sampler));

    VkImageViewCreateInfo imageViewCreateInfo{
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .flags = imageCreateInfo.imageViewInfo.flags,
        .image = newImage->image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = imageCreateInfo.format,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        }};
    VK_CHECK_RESULT(vkCreateImageView(logicalDevice, &imageViewCreateInfo,
                                      nullptr, &newImage->view));

    newImage->descriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    newImage->descriptor.sampler = newImage->sampler;
    newImage->descriptor.imageView = newImage->view;
    newImage->offset = imageCreateInfo.offset;

    VkMemoryRequirements memReq;
    vkGetImageMemoryRequirements(logicalDevice, newImage->image, &memReq);
    uint32_t newImageIndex = newImages.size();
    bool allocated = false;

    for (auto reservation : resourceReservations) {
      if (reservation.tryReserve(memReq, imageCreateInfo.resourceLifespan,
                                 newImageIndex)) {
        allocated = true;
        break;
      }
    }

    if (!allocated) {
      resourceReservations.push_back(ResourceReservation(
          memReq, imageCreateInfo.resourceLifespan, newImageIndex));
    }

    newImages.push_back(newImage);
  }

  VmaAllocationCreateInfo allocCreateInfo = {
      .flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
      .usage = VMA_MEMORY_USAGE_AUTO,
      .preferredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
      // .allocCreateInfo.priority = 1.0f; Read more about this to better
      // understand advantages
  };

  for (auto &reservation : resourceReservations) {
    VmaAllocation alloc;
    VK_CHECK_RESULT(vmaAllocateMemory(allocator, &reservation.localMemReq,
                                      &allocCreateInfo, &alloc, nullptr));

    for (auto &index : reservation.resourceIndices) {
      newImages[index.first]->deviceMemory = alloc;
      VK_CHECK_RESULT(vmaBindImageMemory2(allocator, alloc, index.second,
                                          newImages[index.first]->image,
                                          nullptr));
    }
  }

  return newImages;
}

VulkanDevice::ResourceReservation::ResourceReservation(
    VkMemoryRequirements memReq, unsigned long range, uint32_t imgIndex) {
  this->localMemReq = memReq;
  memory = std::vector<MemoryReservation>(64, MemoryReservation(memReq.size));

  tryReserve(memReq, range, imgIndex);
}

bool VulkanDevice::ResourceReservation::tryReserve(VkMemoryRequirements memReq,
                                                   unsigned long range,
                                                   uint32_t imgIndex) {
  if (localMemReq.size < memReq.size) return false;

  unsigned long firstUse, lastUse;
  BitScanForward(&firstUse, range);
  BitScanReverse(&lastUse, range);

  for (auto it = memory.begin(); it != memory.end(); it++) {
    for (auto &freeBlock : it->freeStorage) {
      if (freeBlock.size >= memReq.size) {
        uint32_t offset = freeBlock.offset;
        it->usedStorage.push_back(MemoryBlock(memReq.size, offset));
        freeBlock.size -= memReq.size;
        freeBlock.offset = offset + memReq.size;
        if (freeBlock.size == 0) {
          memory.erase(it);
        }
        resourceIndices.push_back(std::make_pair(imgIndex, offset));
        return true;
      }
    }
  }

  return false;
}
}  // namespace core_internal::rendering::vulkan