#include "BaseRenderer.h"

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

  vkFreeCommandBuffers(device, cmdPool,
                       static_cast<uint32_t>(drawCmdBuffers.size()),
                       drawCmdBuffers.data());
  vkDestroyCommandPool(device, cmdPool, nullptr);

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

void BaseRenderer::setupFrameBuffer() {}

void BaseRenderer::setupRenderPass() {}

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

void BaseRenderer::createCommandPool() {
  VkCommandPoolCreateInfo cmdPoolInfo = {};
  cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  cmdPoolInfo.queueFamilyIndex = swapChain.queueNodeIndex;
  cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  VK_CHECK_RESULT(vkCreateCommandPool(device, &cmdPoolInfo, nullptr, &cmdPool));
}

void BaseRenderer::createCommandBuffers() {
  // Create one command buffer for each swap chain image and reuse for rendering
  drawCmdBuffers.resize(swapChain.imageCount);

  VkCommandBufferAllocateInfo cmdBufAllocateInfo =
      vks::initializers::commandBufferAllocateInfo(
          cmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY,
          static_cast<uint32_t>(drawCmdBuffers.size()));

  VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &cmdBufAllocateInfo,
                                           drawCmdBuffers.data()));
}

void BaseRenderer::destroyCommandBuffers() {
  vkFreeCommandBuffers(device, cmdPool,
                       static_cast<uint32_t>(drawCmdBuffers.size()),
                       drawCmdBuffers.data());
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

  // Select physical device to be used for the Vulkan example
  // Defaults to the first device unless specified by command line
  uint32_t selectedDevice = 0;

  physicalDevice = physicalDevices[selectedDevice];

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

  // Get a graphics queue from the device
  vkGetDeviceQueue(device, vulkanDevice->queueFamilyIndices.graphics, 0,
                   &queue);

  // TODO
  // Find a suitable depth and/or stencil format
  VkBool32 validFormat{false};
  validFormat =
      vks::tools::getSupportedDepthFormat(physicalDevice, &depthFormat);

  assert(validFormat);

  swapChain.connect(instance, physicalDevice, device);

  return true;
}

void BaseRenderer::setupWindow() {
  window = new WindowManager(title.c_str(), width, height);
}

void BaseRenderer::prepare() {
  initSwapchain();
  createCommandPool();
  setupSwapChain();
  createCommandBuffers();
  createSynchronizationPrimitives();
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
  VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo,
                                semaphores.inFlightFences[currentFrameIndex]));

  VkResult result =
      swapChain.queuePresent(queue, currentFrameIndex, signalSemaphores);
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

// ERROR: [1219306694][VUID-VkPresentInfoKHR-pImageIndices-01430] : Validation
// Error: [ VUID-VkPresentInfoKHR-pImageIndices-01430 ] Object 0: handle =
// 0x95ff2600000000b7, type = VK_OBJECT_TYPE_SWAPCHAIN_KHR; | MessageID =
// 0x48ad24c6 | vkQueuePresentKHR: pSwapchains[0] image at index 1 was not
// acquired from the swapchain. The Vulkan spec states: Each element of
// pImageIndices must be the index of a presentable image acquired from the
// swapchain specified by the corresponding element of the pSwapchains array,
// and the presented image subresource must be in the
// VK_IMAGE_LAYOUT_PRESENT_SRC_KHR or VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR layout
// at the time the operation is executed on a VkDevice
// (https://vulkan.lunarg.com/doc/view/1.3.261.1/windows/1.3-extensions/vkspec.html#VUID-VkPresentInfoKHR-pImageIndices-01430)
void BaseRenderer::setSampleCount(VkSampleCountFlagBits sampleCount) {
  if ((deviceProperties.limits.framebufferColorSampleCounts & sampleCount) &&
      deviceProperties.limits.framebufferDepthSampleCounts & sampleCount) {
    this->sampleCount = sampleCount;
  } else {
    std::cerr << "Sample Count of " << sampleCount << " is unsupported"
              << std::endl;
  }
}

VkSampleCountFlagBits BaseRenderer::getSampleCount() { return sampleCount; }

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