#include "BaseRenderer.h"

#include <map>

#include "../Debug/VulkanDebug.h"
#include "../ResourceManagement/VulkanResources/VulkanTools.h"

BaseRenderer::BaseRenderer() {
  struct stat info;
  if (stat(getAssetPath().c_str(), &info) != 0) {
    std::string msg =
        "Could not locate asset path in \"" + getAssetPath() + "\" !";
    MessageBox(NULL, msg.c_str(), "Fatal error", MB_OK | MB_ICONERROR);
    exit(-1);
  }
}

BaseRenderer::~BaseRenderer() {
  // Clean up Vulkan resources
  swapChain.cleanup();
  if (descriptorPool != VK_NULL_HANDLE) {
    vkDestroyDescriptorPool(device, descriptorPool, nullptr);
  }

  vkDestroyImage(device, depthStencil.image, nullptr);
  vkDestroyImageView(device, depthStencil.view, nullptr);
  vkFreeMemory(device, depthStencil.memory, nullptr);

  if (renderPass != VK_NULL_HANDLE) {
    vkDestroyRenderPass(device, renderPass, nullptr);
  }
  for (uint32_t i = 0; i < frameBuffers.size(); i++) {
    vkDestroyFramebuffer(device, frameBuffers[i], nullptr);
  }

  for (auto& shaderModule : shaderModules) {
    vkDestroyShaderModule(device, shaderModule, nullptr);
  }

  vkDestroyPipelineCache(device, pipelineCache, nullptr);

  vkFreeCommandBuffers(device, graphicsCmdPool,
                       static_cast<uint32_t>(drawCmdBuffers.size()),
                       drawCmdBuffers.data());
  vkDestroyCommandPool(device, graphicsCmdPool, nullptr);

  vkFreeCommandBuffers(device, computeCmdPool,
                       static_cast<uint32_t>(computeCmdBuffers.size()),
                       computeCmdBuffers.data());
  vkDestroyCommandPool(device, computeCmdPool, nullptr);

  destroySynchronizationPrimitives();

  delete vulkanDevice;

  if (settings.validation) {
    vks::debug::freeDebugCallback(instance);
  }

  vkDestroyInstance(instance, nullptr);
}

void BaseRenderer::start() {
  initVulkan();
  setupWindow();
  prepare();
  renderLoop();
}

void BaseRenderer::initSwapchain() {
  swapChain.initSurface(window->getWindow());
}

void BaseRenderer::setupSwapChain() {
  swapChain.create(&width, &height, settings.vsync, settings.fullscreen);
}

VkResult BaseRenderer::createInstance() {
  // Validation can also be forced via a define
#if defined(_VALIDATION)
  this->settings.validation = true;
#endif

  VkApplicationInfo appInfo = {};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = name.c_str();
  appInfo.pEngineName = name.c_str();
  appInfo.apiVersion = apiVersion;

  std::vector<const char*> instanceExtensions = {
      VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME};

  // Get extensions supported by the instance and store for later use
  uint32_t extCount = 0;
  vkEnumerateInstanceExtensionProperties(nullptr, &extCount, nullptr);
  if (extCount > 0) {
    std::vector<VkExtensionProperties> extensions(extCount);
    if (vkEnumerateInstanceExtensionProperties(
            nullptr, &extCount, &extensions.front()) == VK_SUCCESS) {
      for (VkExtensionProperties& extension : extensions) {
        supportedInstanceExtensions.push_back(extension.extensionName);
      }
    }
  }

  // Enabled requested instance extensions
  if (enabledInstanceExtensions.size() > 0) {
    for (const char* enabledExtension : enabledInstanceExtensions) {
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
  if (settings.validation) {
    vks::debug::setupDebugingMessengerCreateInfo(debugUtilsMessengerCI);
    debugUtilsMessengerCI.pNext = instanceCreateInfo.pNext;
    instanceCreateInfo.pNext = &debugUtilsMessengerCI;
  }

  // Enable the debug utils extension if available (e.g. when debugging tools
  // are present)
  if (settings.validation || std::find(supportedInstanceExtensions.begin(),
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
  const char* validationLayerName = "VK_LAYER_KHRONOS_validation";
  if (settings.validation) {
    // Check if this layer is available at instance level
    uint32_t instanceLayerCount;
    vkEnumerateInstanceLayerProperties(&instanceLayerCount, nullptr);
    std::vector<VkLayerProperties> instanceLayerProperties(instanceLayerCount);
    vkEnumerateInstanceLayerProperties(&instanceLayerCount,
                                       instanceLayerProperties.data());
    bool validationLayerPresent = false;
    for (VkLayerProperties& layer : instanceLayerProperties) {
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

  VkResult result = vkCreateInstance(&instanceCreateInfo, nullptr, &instance);

  // If the debug utils extension is present we set up debug functions, so
  // samples can label objects for debugging
  if (std::find(supportedInstanceExtensions.begin(),
                supportedInstanceExtensions.end(),
                VK_EXT_DEBUG_UTILS_EXTENSION_NAME) !=
      supportedInstanceExtensions.end()) {
    vks::debugutils::setup(instance);
  }

  return result;
}

VkPipelineShaderStageCreateInfo BaseRenderer::loadShader(
    std::string fileName, VkShaderStageFlagBits stage) {
  VkPipelineShaderStageCreateInfo shaderStage = {};
  shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shaderStage.stage = stage;
  shaderStage.module = vks::tools::loadShader(fileName.c_str(), device);
  shaderStage.pName = "main";
  assert(shaderStage.module != VK_NULL_HANDLE);
  shaderModules.push_back(shaderStage.module);
  return shaderStage;
}

void BaseRenderer::createPipelineCache() {
  VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
  pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
  VK_CHECK_RESULT(vkCreatePipelineCache(device, &pipelineCacheCreateInfo,
                                        nullptr, &pipelineCache));
}

void BaseRenderer::setupFrameBuffer() {
  std::array<VkImageView, 2> attachments;
  // attachment[0]
  // Depth/Stencil attachment is the same for all frame buffers
  attachments[1] = depthStencil.view;

  VkFramebufferCreateInfo frameBufferCreateInfo = {};
  frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  frameBufferCreateInfo.pNext = NULL;
  frameBufferCreateInfo.renderPass = renderPass;
  frameBufferCreateInfo.attachmentCount =
      static_cast<uint32_t>(attachments.size());
  frameBufferCreateInfo.pAttachments = attachments.data();
  frameBufferCreateInfo.width = width;
  frameBufferCreateInfo.height = height;
  frameBufferCreateInfo.layers = 1;

  // Create frame buffers for every swap chain image
  frameBuffers.resize(swapChain.imageCount);
  for (uint32_t i = 0; i < frameBuffers.size(); i++) {
    attachments[0] = swapChain.buffers[i].view;
    VK_CHECK_RESULT(vkCreateFramebuffer(device, &frameBufferCreateInfo, nullptr,
                                        &frameBuffers[i]));
  }
}

void BaseRenderer::setupRenderPass() {
  std::array<VkAttachmentDescription, 2> attachments = {};
  // Color attachment
  attachments[0].format = swapChain.colorFormat;
  attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
  attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
  // Depth attachment
  attachments[1].format = depthFormat;
  attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
  attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkAttachmentReference colorReference = {};
  colorReference.attachment = 0;
  colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkAttachmentReference depthReference = {};
  depthReference.attachment = 1;
  depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpassDescription = {};
  subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpassDescription.colorAttachmentCount = 1;
  subpassDescription.pColorAttachments = &colorReference;
  subpassDescription.pDepthStencilAttachment = &depthReference;
  subpassDescription.inputAttachmentCount = 0;
  subpassDescription.pInputAttachments = nullptr;
  subpassDescription.preserveAttachmentCount = 0;
  subpassDescription.pPreserveAttachments = nullptr;
  subpassDescription.pResolveAttachments = nullptr;

  // Subpass dependencies for layout transitions
  std::array<VkSubpassDependency, 2> dependencies;

  dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
  dependencies[0].dstSubpass = 0;
  dependencies[0].srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                                 VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
  dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                                 VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
  dependencies[0].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
  dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |
                                  VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
  dependencies[0].dependencyFlags = 0;

  dependencies[1].srcSubpass = VK_SUBPASS_EXTERNAL;
  dependencies[1].dstSubpass = 0;
  dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependencies[1].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependencies[1].srcAccessMask = 0;
  dependencies[1].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                                  VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
  dependencies[1].dependencyFlags = 0;

  VkRenderPassCreateInfo renderPassInfo = {};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
  renderPassInfo.pAttachments = attachments.data();
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpassDescription;
  renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
  renderPassInfo.pDependencies = dependencies.data();

  VK_CHECK_RESULT(
      vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass));
}

void BaseRenderer::setupDepthStencil() {
  VkImageCreateInfo imageCI{};
  imageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageCI.imageType = VK_IMAGE_TYPE_2D;
  imageCI.format = depthFormat;
  imageCI.extent = {width, height, 1};
  imageCI.mipLevels = 1;
  imageCI.arrayLayers = 1;
  imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
  imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
  imageCI.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

  VK_CHECK_RESULT(
      vkCreateImage(device, &imageCI, nullptr, &depthStencil.image));
  VkMemoryRequirements memReqs{};
  vkGetImageMemoryRequirements(device, depthStencil.image, &memReqs);

  VkMemoryAllocateInfo memAllloc{};
  memAllloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  memAllloc.allocationSize = memReqs.size;
  memAllloc.memoryTypeIndex = vulkanDevice->getMemoryType(
      memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  VK_CHECK_RESULT(
      vkAllocateMemory(device, &memAllloc, nullptr, &depthStencil.memory));
  VK_CHECK_RESULT(
      vkBindImageMemory(device, depthStencil.image, depthStencil.memory, 0));

  VkImageViewCreateInfo imageViewCI{};
  imageViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  imageViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
  imageViewCI.image = depthStencil.image;
  imageViewCI.format = depthFormat;
  imageViewCI.subresourceRange.baseMipLevel = 0;
  imageViewCI.subresourceRange.levelCount = 1;
  imageViewCI.subresourceRange.baseArrayLayer = 0;
  imageViewCI.subresourceRange.layerCount = 1;
  imageViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
  // Stencil aspect should only be set on depth + stencil formats
  // (VK_FORMAT_D16_UNORM_S8_UINT..VK_FORMAT_D32_SFLOAT_S8_UINT
  if (depthFormat >= VK_FORMAT_D16_UNORM_S8_UINT) {
    imageViewCI.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
  }
  VK_CHECK_RESULT(
      vkCreateImageView(device, &imageViewCI, nullptr, &depthStencil.view));
}

void BaseRenderer::createSynchronizationPrimitives() {
  semaphores.imageAvailableSemaphores.resize(swapChain.imageCount);
  semaphores.renderFinishedSemaphores.resize(swapChain.imageCount);
  semaphores.inFlightFences.resize(swapChain.imageCount);

  VkSemaphoreCreateInfo semaphoreInfo{};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fenceInfo{};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  for (size_t i = 0; i < swapChain.imageCount; i++) {
    if (vkCreateSemaphore(device, &semaphoreInfo, nullptr,
                          &semaphores.imageAvailableSemaphores[i]) !=
            VK_SUCCESS ||
        vkCreateSemaphore(device, &semaphoreInfo, nullptr,
                          &semaphores.renderFinishedSemaphores[i]) !=
            VK_SUCCESS ||
        vkCreateFence(device, &fenceInfo, nullptr,
                      &semaphores.inFlightFences[i]) != VK_SUCCESS) {
      throw std::runtime_error(
          "failed to create synchronization objects for a frame!");
    }
  }
}

void BaseRenderer::destroySynchronizationPrimitives() {
  for (size_t i = 0; i < swapChain.imageCount; i++) {
    vkDestroySemaphore(device, semaphores.renderFinishedSemaphores[i], nullptr);
    vkDestroySemaphore(device, semaphores.imageAvailableSemaphores[i], nullptr);
    vkDestroyFence(device, semaphores.inFlightFences[i], nullptr);
  }
}

void BaseRenderer::createCommandPools() {
  VkCommandPoolCreateInfo graphicsCmdPoolInfo = {};
  graphicsCmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  graphicsCmdPoolInfo.queueFamilyIndex = swapChain.graphicsQueueNodeIndex;
  graphicsCmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  VK_CHECK_RESULT(vkCreateCommandPool(device, &graphicsCmdPoolInfo, nullptr,
                                      &graphicsCmdPool));

  VkCommandPoolCreateInfo computeCmdPoolInfo = {};
  computeCmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  computeCmdPoolInfo.queueFamilyIndex = swapChain.computeQueueNodeIndex;
  computeCmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  VK_CHECK_RESULT(vkCreateCommandPool(device, &computeCmdPoolInfo, nullptr,
                                      &computeCmdPool));
}

void BaseRenderer::createCommandBuffers() {
  // Create one command buffer for each swap chain image and reuse for rendering
  drawCmdBuffers.resize(swapChain.imageCount);

  VkCommandBufferAllocateInfo graphicsCmdBufAllocateInfo =
      vks::initializers::commandBufferAllocateInfo(
          graphicsCmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY,
          static_cast<uint32_t>(drawCmdBuffers.size()));

  VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &graphicsCmdBufAllocateInfo,
                                           drawCmdBuffers.data()));
  computeCmdBuffers.resize(swapChain.imageCount);

  VkCommandBufferAllocateInfo computeCmdBufAllocateInfo =
      vks::initializers::commandBufferAllocateInfo(
          computeCmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY,
          static_cast<uint32_t>(computeCmdBuffers.size()));

  VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &computeCmdBufAllocateInfo,
                                           computeCmdBuffers.data()));
}

void BaseRenderer::destroyCommandBuffers() {
  vkFreeCommandBuffers(device, graphicsCmdPool,
                       static_cast<uint32_t>(drawCmdBuffers.size()),
                       drawCmdBuffers.data());
  vkFreeCommandBuffers(device, computeCmdPool,
                       static_cast<uint32_t>(computeCmdBuffers.size()),
                       computeCmdBuffers.data());
}

bool BaseRenderer::initVulkan() {
  VkResult err;

  // Vulkan instance
  err = createInstance();
  if (err) {
    vks::tools::exitFatal(
        "Could not create Vulkan instance : \n" + vks::tools::errorString(err),
        err);
    return false;
  }

  // If requested, we enable the default validation layers for debugging
  if (settings.validation) {
    vks::debug::setupDebugging(instance);
  }

  // Physical device
  uint32_t gpuCount = 0;
  // Get number of available physical devices
  VK_CHECK_RESULT(vkEnumeratePhysicalDevices(instance, &gpuCount, nullptr));
  if (gpuCount == 0) {
    vks::tools::exitFatal("No device with Vulkan support found", -1);
    return false;
  }
  // Enumerate devices
  std::vector<VkPhysicalDevice> physicalDevices(gpuCount);
  err = vkEnumeratePhysicalDevices(instance, &gpuCount, physicalDevices.data());
  if (err) {
    vks::tools::exitFatal("Could not enumerate physical devices : \n" +
                              vks::tools::errorString(err),
                          err);
    return false;
  }

  // GPU selection
  physicalDevice = choosePhysicalDevice(physicalDevices);

  // Store properties (including limits), features and memory properties of the
  // physical device (so that examples can check against them)
  vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
  vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);
  vkGetPhysicalDeviceMemoryProperties(physicalDevice, &deviceMemoryProperties);

  // Derived examples can override this to set actual features (based on above
  // readings) to enable for logical device creation
  getEnabledFeatures();

  // Vulkan device creation
  // This is handled by a separate class that gets a logical device
  // representation and encapsulates functions related to a device
  vulkanDevice = new vks::VulkanDevice(physicalDevice);

  // Derived examples can enable extensions based on the list of supported
  // extensions read from the physical device
  getEnabledExtensions();

  VkResult res = vulkanDevice->createLogicalDevice(
      enabledFeatures, enabledDeviceExtensions, deviceCreatepNextChain);
  if (res != VK_SUCCESS) {
    vks::tools::exitFatal(
        "Could not create Vulkan device: \n" + vks::tools::errorString(res),
        res);
    return false;
  }
  device = vulkanDevice->logicalDevice;

  vkGetDeviceQueue(device, vulkanDevice->queueFamilyIndices.graphics, 0,
                   &graphicsQueue);
  vkGetDeviceQueue(device, vulkanDevice->queueFamilyIndices.compute, 0,
                   &computeQueue);

  // TODO
  // Find a suitable depth and/or stencil format
  VkBool32 validFormat{false};
  validFormat =
      vks::tools::getSupportedDepthFormat(physicalDevice, &depthFormat);

  assert(validFormat);

  swapChain.connect(instance, physicalDevice, device);

  return true;
}

VkPhysicalDevice BaseRenderer::choosePhysicalDevice(
    std::vector<VkPhysicalDevice> devices) {
  std::multimap<int, VkPhysicalDevice> candidates;

  for (auto& device : devices) {
    int score = rateDeviceSuitability(device);
    candidates.insert(std::make_pair(score, device));
  }

  if (candidates.rbegin()->first > 0) {
    return candidates.rbegin()->second;
  } else {
    throw std::runtime_error("failed to find a suitable GPU!");
  }
}

int BaseRenderer::rateDeviceSuitability(VkPhysicalDevice device) {
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

void BaseRenderer::setupWindow() {
  window = new WindowManager((title + name).c_str(), width, height);
}

void BaseRenderer::prepare() {
  initSwapchain();
  createCommandPools();
  setupSwapChain();
  createCommandBuffers();
  createSynchronizationPrimitives();
  setupDepthStencil();
  setupRenderPass();
  createPipelineCache();
  setupFrameBuffer();
}

void BaseRenderer::nextFrame() {
  auto tStart = std::chrono::high_resolution_clock::now();
  if (viewUpdated) {
    viewUpdated = false;
  }
  window->handleEvents();
  polledEvents(window->getWindow());
  render();
  frameCounter++;
  currentFrameIndex = (currentFrameIndex + 1) % swapChain.imageCount;
  auto tEnd = std::chrono::high_resolution_clock::now();
  auto tDiff = std::chrono::duration<double, std::milli>(tEnd - tStart).count();

  frameTimer = (float)tDiff / 1000.0f;
  camera.update(frameTimer);
  if (camera.moving()) {
    viewUpdated = true;
  }
  // Convert to clamped timer value
  if (!paused) {
    timer += timerSpeed * frameTimer;
    if (timer > 1.0) {
      timer -= 1.0f;
    }
  }
  float fpsTimer =
      (float)(std::chrono::duration<double, std::milli>(tEnd - lastTimestamp)
                  .count());
  if (fpsTimer > 1000.0f) {
    lastFPS = static_cast<uint32_t>((float)frameCounter * (1000.0f / fpsTimer));

    frameCounter = 0;
    lastTimestamp = tEnd;
  }
  tPrevEnd = tEnd;
}

void BaseRenderer::renderLoop() {
  lastTimestamp = std::chrono::high_resolution_clock::now();
  tPrevEnd = lastTimestamp;

  MSG msg;
  bool quitMessageReceived = false;
  while (!quitMessageReceived) {
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
      if (msg.message == WM_QUIT) {
        quitMessageReceived = true;
        break;
      }
    }
    if (prepared) {
      nextFrame();
    }
  }
  // Flush device to make sure all resources can be freed
  if (device != VK_NULL_HANDLE) {
    vkDeviceWaitIdle(device);
  }
}

void BaseRenderer::prepareFrame() {
  vkWaitForFences(device, 1, &semaphores.inFlightFences[currentFrameIndex],
                  VK_TRUE, UINT64_MAX);
  // Acquire the next image from the swap chain
  VkResult result = swapChain.acquireNextImage(
      semaphores.imageAvailableSemaphores[currentFrameIndex],
      &currentImageIndex);
  // Recreate the swapchain if it's no longer compatible with the surface
  // (OUT_OF_DATE) SRS - If no longer optimal (VK_SUBOPTIMAL_KHR), wait until
  // submitFrame() in case number of swapchain images will change on resize
  if ((result == VK_ERROR_OUT_OF_DATE_KHR) || (result == VK_SUBOPTIMAL_KHR)) {
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
      windowResize();
    }
    return;
  } else {
    VK_CHECK_RESULT(result);
  }

  vkResetFences(device, 1, &semaphores.inFlightFences[currentFrameIndex]);
}

void BaseRenderer::submitFrame() {
  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  VkSemaphore waitSemaphores[] = {
      semaphores.imageAvailableSemaphores[currentFrameIndex]};
  VkPipelineStageFlags waitStages[] = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = waitSemaphores;
  submitInfo.pWaitDstStageMask = waitStages;

  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &drawCmdBuffers[currentFrameIndex];

  VkSemaphore signalSemaphores[] = {
      semaphores.renderFinishedSemaphores[currentFrameIndex]};
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = signalSemaphores;
  VK_CHECK_RESULT(vkQueueSubmit(graphicsQueue, 1, &submitInfo,
                                semaphores.inFlightFences[currentFrameIndex]));

  VkResult result = swapChain.queuePresent(graphicsQueue, currentFrameIndex,
                                           signalSemaphores);
  // Recreate the swapchain if it's no longer compatible with the surface
  // (OUT_OF_DATE) or no longer optimal for presentation (SUBOPTIMAL)
  if ((result == VK_ERROR_OUT_OF_DATE_KHR) || (result == VK_SUBOPTIMAL_KHR)) {
    windowResize();
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
      return;
    }
  } else {
    VK_CHECK_RESULT(result);
  }
}

void BaseRenderer::setSampleCount(VkSampleCountFlagBits sampleCount) {
  if ((deviceProperties.limits.framebufferColorSampleCounts & sampleCount) &&
      deviceProperties.limits.framebufferDepthSampleCounts & sampleCount) {
    this->msaaSampleCount = sampleCount;
  } else {
    std::cerr << "Sample Count of " << sampleCount << " is unsupported"
              << std::endl;
  }
}

VkSampleCountFlagBits BaseRenderer::getMSAASampleCount() {
  return msaaSampleCount;
}

void BaseRenderer::polledEvents(GLFWwindow* window) {
  double xpos, ypos;
  glfwGetCursorPos(window, &xpos, &ypos);

  if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT)) {
    mouseState.buttons.left = true;
  } else {
    mouseState.buttons.left = false;
  }
  if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT)) {
    mouseState.buttons.right = true;
  } else {
    mouseState.buttons.right = false;
  }
  if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE)) {
    mouseState.buttons.middle = true;
  } else {
    mouseState.buttons.middle = false;
  }
  handleMouseMove(xpos, ypos);

  if (glfwGetKey(window, GLFW_KEY_ESCAPE)) {
    PostQuitMessage(0);
  }

  ImGuiIO& io = ImGui::GetIO();

  if (io.WantCaptureKeyboard && settings.overlay) {
    return;
  }

  if (glfwGetKey(window, GLFW_KEY_P)) {
    paused != paused;
  }
  if (glfwGetKey(window, GLFW_KEY_F2)) {
    if (camera.type == Camera::CameraType::lookat) {
      camera.type = Camera::CameraType::firstperson;
    } else {
      camera.type = Camera::CameraType::lookat;
    }
  }
  if (glfwGetKey(window, GLFW_KEY_W)) {
    camera.keys.forward = true;
  } else {
    camera.keys.forward = false;
  }
  if (glfwGetKey(window, GLFW_KEY_S)) {
    camera.keys.backward = true;
  } else {
    camera.keys.backward = false;
  }
  if (glfwGetKey(window, GLFW_KEY_A)) {
    camera.keys.left = true;
  } else {
    camera.keys.left = false;
  }
  if (glfwGetKey(window, GLFW_KEY_D)) {
    camera.keys.right = true;
  } else {
    camera.keys.right = false;
  }
  if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL)) {
    camera.keys.down = true;
  } else {
    camera.keys.down = false;
  }
  if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT)) {
    camera.keys.up = true;
  } else {
    camera.keys.up = false;
  }
}

void BaseRenderer::buildCommandBuffer() {
  vkResetCommandBuffer(drawCmdBuffers[currentFrameIndex], 0);
}

void BaseRenderer::windowResize() {
  if (!prepared) {
    return;
  }
  prepared = false;
  resized = true;
  currentFrameIndex = 0;
  // Ensure all operations on the device have been finished before destroying
  // resources
  vkDeviceWaitIdle(device);

  // Recreate swap chain
  int destWidth, destHeight;
  glfwGetFramebufferSize(window->getWindow(), &destWidth, &destHeight);
  width = destWidth;
  height = destHeight;

  setupSwapChain();

  // Recreate the frame buffers
  for (uint32_t i = 0; i < frameBuffers.size(); i++) {
    vkDestroyFramebuffer(device, frameBuffers[i], nullptr);
  }
  setupFrameBuffer();

  destroyCommandBuffers();
  createCommandBuffers();

  destroySynchronizationPrimitives();
  createSynchronizationPrimitives();

  vkDeviceWaitIdle(device);

  if ((width > 0.0f) && (height > 0.0f)) {
    camera.updateAspectRatio((float)width / (float)height);
  }

  // Notify derived class
  windowResized();

  prepared = true;
}

void BaseRenderer::handleMouseMove(int32_t x, int32_t y) {
  int32_t dx = (int32_t)mouseState.position.x - x;
  int32_t dy = (int32_t)mouseState.position.y - y;

  bool handled = false;

  ImGuiIO& io = ImGui::GetIO();
  handled = io.WantCaptureMouse && settings.overlay;

  if (handled) {
    mouseState.position = glm::vec2((float)x, (float)y);
    return;
  }

  if (mouseState.buttons.left) {
    camera.rotate(
        glm::vec3(dy * camera.rotationSpeed, -dx * camera.rotationSpeed, 0.0f));
    viewUpdated = true;
  }
  if (mouseState.buttons.right) {
    camera.translate(glm::vec3(-0.0f, 0.0f, dy * .005f));
    viewUpdated = true;
  }
  if (mouseState.buttons.middle) {
    camera.translate(glm::vec3(-dx * 0.005f, -dy * 0.005f, 0.0f));
    viewUpdated = true;
  }
  mouseState.position = glm::vec2((float)x, (float)y);
}

const char* BaseRenderer::getTitle() { return name.c_str(); }

uint32_t BaseRenderer::getWidth() { return width; }

uint32_t BaseRenderer::getHeight() { return height; }

void BaseRenderer::windowResized() {}

void BaseRenderer::getEnabledFeatures() {}

void BaseRenderer::getEnabledExtensions() {}