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

  vkGetDeviceQueue(logicalDevice, queueFamilyIndices.graphics, 0,
                   &queues.graphics);
  vkGetDeviceQueue(logicalDevice, queueFamilyIndices.compute, 0,
                   &queues.compute);
  vkGetDeviceQueue(logicalDevice, queueFamilyIndices.transfer, 0,
                   &queues.transfer);

  VmaAllocatorCreateInfo vmaAllocInfo{};
  vmaAllocInfo.device = logicalDevice;
  vmaAllocInfo.physicalDevice = physicalDevice;
  vmaAllocInfo.instance = instance;
  vmaAllocInfo.vulkanApiVersion = apiVersion;
  vmaAllocInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

  vmaCreateAllocator(&vmaAllocInfo, &allocator);

  VkCommandPoolCreateInfo cmdPoolInfo = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .flags = 0,
      .queueFamilyIndex = queueFamilyIndices.graphics,
  };

  VK_CHECK_RESULT(
      vkCreateCommandPool(logicalDevice, &cmdPoolInfo, nullptr, &commandPool));
}

VulkanDevice::~VulkanDevice() {
  for (auto &shaderModule : shaderModules) {
    vkDestroyShaderModule(logicalDevice, shaderModule, nullptr);
  }

  if (commandPool) {
    vkDestroyCommandPool(logicalDevice, commandPool, nullptr);
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

VkCommandBuffer VulkanDevice::createCommandBuffer(VkCommandBufferLevel level,
                                                  VkCommandPool pool,
                                                  bool begin) {
  VkCommandBufferAllocateInfo cmdBufAllocateInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool = pool,
      .level = level,
      .commandBufferCount = 1,
  };

  VkCommandBuffer cmdBuffer;
  VK_CHECK_RESULT(
      vkAllocateCommandBuffers(logicalDevice, &cmdBufAllocateInfo, &cmdBuffer));
  // If requested, also start recording for the new command buffer
  if (begin) {
    VkCommandBufferBeginInfo cmdBufInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuffer, &cmdBufInfo));
  }
  return cmdBuffer;
}

VkCommandBuffer VulkanDevice::createCommandBuffer(VkCommandBufferLevel level,
                                                  bool begin) {
  return createCommandBuffer(level, commandPool, begin);
}

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

Image *VulkanDevice::createImageFromBuffer(ImageInfo &imageCreateInfo,
                                           void **data, VkDeviceSize dataSize) {
  if (!(imageCreateInfo.usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT)) {
    imageCreateInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  }

  auto newImage = createImage(imageCreateInfo, false);

  VkBufferCreateInfo bufCreateInfo{
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .size = dataSize,
      .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
  };

  VmaAllocationCreateInfo allocCreateInfo{
      .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
               VMA_ALLOCATION_CREATE_MAPPED_BIT,
      .usage = VMA_MEMORY_USAGE_AUTO,
  };

  VkBuffer stagingBuffer;
  VmaAllocation alloc;
  VmaAllocationInfo allocInfo;

  vmaCreateBuffer(allocator, &bufCreateInfo, &allocCreateInfo, &stagingBuffer,
                  &alloc, &allocInfo);

  memcpy(allocInfo.pMappedData, data, dataSize);

  VkBufferImageCopy bufferCopyRegion = {};
  bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  bufferCopyRegion.imageSubresource.mipLevel = imageCreateInfo.mipLevels;
  bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
  bufferCopyRegion.imageSubresource.layerCount = imageCreateInfo.arrayLayers;
  bufferCopyRegion.imageExtent.width = imageCreateInfo.width;
  bufferCopyRegion.imageExtent.height = imageCreateInfo.height;
  bufferCopyRegion.imageExtent.depth = imageCreateInfo.depth;
  bufferCopyRegion.bufferOffset = 0;

  auto copyCmd = createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

  newImage->transitionImageLayout(copyCmd,
                                  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

  vkCmdCopyBufferToImage(copyCmd, stagingBuffer, newImage->image,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                         &bufferCopyRegion);

  newImage->transitionImageLayout(copyCmd,
                                  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  VK_CHECK_RESULT(vkEndCommandBuffer(copyCmd));

  VkSubmitInfo submitInfo{
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .commandBufferCount = 1,
      .pCommandBuffers = &copyCmd,
  };

  VkFenceCreateInfo fenceInfo{.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
                              .flags = VK_FLAGS_NONE};

  VkFence fence;
  VK_CHECK_RESULT(vkCreateFence(logicalDevice, &fenceInfo, nullptr, &fence));
  VK_CHECK_RESULT(vkQueueSubmit(queues.transfer, 1, &submitInfo, fence));
  VK_CHECK_RESULT(vkWaitForFences(logicalDevice, 1, &fence, VK_TRUE,
                                  DEFAULT_FENCE_TIMEOUT));
  vkDestroyFence(logicalDevice, fence, nullptr);
  vkFreeCommandBuffers(logicalDevice, commandPool, 1, &copyCmd);

  vmaDestroyBuffer(allocator, stagingBuffer, alloc);

  return newImage;
}

Image *VulkanDevice::createImage(const ImageInfo &imageCreateInfo,
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
      .preferredFlags = imageCreateInfo.memoryFlags,
  };

  if (renderResource)
    allocCreateInfo.flags |= VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
  if (imageCreateInfo.requireMappedData)
    allocCreateInfo.flags |= VMA_ALLOCATION_CREATE_MAPPED_BIT;

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
    newImage->subresourceRange = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1,
    };
    VkImageViewCreateInfo imageViewCreateInfo{
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .flags = imageCreateInfo.imageViewInfo.flags,
        .image = newImage->image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = imageCreateInfo.format,
        .subresourceRange = newImage->subresourceRange,
    };
    VK_CHECK_RESULT(vkCreateImageView(logicalDevice, &imageViewCreateInfo,
                                      nullptr, &newImage->view));
  }

  newImage->descriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  newImage->descriptor.sampler = newImage->sampler;
  newImage->descriptor.imageView = newImage->view;

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
    newImage->subresourceRange = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1,
    };

    VkImageViewCreateInfo imageViewCreateInfo{
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .flags = imageCreateInfo.imageViewInfo.flags,
        .image = newImage->image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = imageCreateInfo.format,
        .subresourceRange = newImage->subresourceRange,
    };
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

    // index.first is resource index
    // index.second is resource memory offset
    for (auto &index : reservation.resourceIndices) {
      newImages[index.first]->deviceMemory = alloc;
      VK_CHECK_RESULT(vmaBindImageMemory2(allocator, alloc, index.second,
                                          newImages[index.first]->image,
                                          nullptr));
    }
  }

  return newImages;
}

Buffer *VulkanDevice::createBuffer(const BufferInfo &bufferCreateInfo,
                                   bool renderResource) {
  auto newBuffer = new Buffer();
  VmaAllocationCreateInfo allocCreateInfo = {
      .usage = VMA_MEMORY_USAGE_AUTO,
      .preferredFlags = bufferCreateInfo.memoryFlags,
  };

  if (renderResource)
    allocCreateInfo.flags |= VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
  if (bufferCreateInfo.requireMappedData)
    allocCreateInfo.flags |= VMA_ALLOCATION_CREATE_MAPPED_BIT;

  VkBufferCreateInfo bufInfo{
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .size = bufferCreateInfo.size,
      .usage =
          bufferCreateInfo.usage | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
  };

  VmaAllocationInfo info;
  vmaCreateBuffer(allocator, &bufInfo, &allocCreateInfo, &newBuffer->buffer,
                  &newBuffer->deviceMemory, &info);
  newBuffer->mappedData = info.pMappedData;
  newBuffer->descriptor = {newBuffer->buffer, newBuffer->offset,
                           newBuffer->size};

  VkBufferDeviceAddressInfo addressInfo{
      .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
      .pNext = nullptr,
      .buffer = newBuffer->buffer,
  };

  newBuffer->deviceAddress = static_cast<DeviceAddress>(
      vkGetBufferDeviceAddress(logicalDevice, &addressInfo));

  return newBuffer;
}

std::vector<Buffer *> VulkanDevice::createAliasedBuffers(
    std::vector<BufferInfo> bufferCreateInfos) {
  std::vector<Buffer *> newBuffers;
  std::vector<ResourceReservation> resourceReservations;

  for (auto &bufferCreateInfo : bufferCreateInfos) {
    auto newBuffer = new Buffer();
    VkBufferCreateInfo bufInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = bufferCreateInfo.size,
        .usage = bufferCreateInfo.usage,
    };

    vkCreateBuffer(logicalDevice, &bufInfo, nullptr, &newBuffer->buffer);
    VkMemoryRequirements memReq;
    vkGetBufferMemoryRequirements(logicalDevice, newBuffer->buffer, &memReq);
    uint32_t newBufferIndex = newBuffers.size();
    bool allocated = false;
    for (auto reservation : resourceReservations) {
      if (reservation.tryReserve(memReq, bufferCreateInfo.resourceLifespan,
                                 newBufferIndex)) {
        allocated = true;
        break;
      }
    }

    if (!allocated) {
      resourceReservations.push_back(ResourceReservation(
          memReq, bufferCreateInfo.resourceLifespan, newBufferIndex));
    }

    newBuffer->descriptor = {newBuffer->buffer, newBuffer->offset,
                             newBuffer->size};
    newBuffers.push_back(newBuffer);
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
      newBuffers[index.first]->deviceMemory = alloc;
      VK_CHECK_RESULT(vmaBindBufferMemory2(allocator, alloc, index.second,
                                           newBuffers[index.first]->buffer,
                                           nullptr));
    }
  }

  return newBuffers;
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