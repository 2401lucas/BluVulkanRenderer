#include "BaseRenderer.h"

#include "../Debug/VulkanDebug.h"
#include "../ResourceManagement/VulkanResources/VulkanTools.h"

VkResult BaseRenderer::createInstance(bool enableValidation) {
  this->settings.validation = enableValidation;
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

void BaseRenderer::renderFrame() {
  BaseRenderer::prepareFrame();
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &drawCmdBuffers[currentBuffer];
  VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
  BaseRenderer::submitFrame();
}

std::string BaseRenderer::getWindowTitle() {
  std::string device(deviceProperties.deviceName);
  std::string windowTitle;
  windowTitle = title + " - " + device;
  if (!settings.overlay) {
    windowTitle += " - " + std::to_string(frameCounter) + " fps";
  }
  return windowTitle;
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

void BaseRenderer::createPipelineCache() {
  VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
  pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
  VK_CHECK_RESULT(vkCreatePipelineCache(device, &pipelineCacheCreateInfo,
                                        nullptr, &pipelineCache));
}

void BaseRenderer::prepare() {
  initSwapchain();
  createCommandPool();
  setupSwapChain();
  createCommandBuffers();
  createSynchronizationPrimitives();
  setupDepthStencil();
  setupRenderPass();
  createPipelineCache();
  setupFrameBuffer();
  settings.overlay = settings.overlay && (!benchmark.active);
  if (settings.overlay) {
    UIOverlay.device = vulkanDevice;
    UIOverlay.queue = queue;
    UIOverlay.shaders = {
        loadShader("shaders/uioverlay.vert.spv", VK_SHADER_STAGE_VERTEX_BIT),
        loadShader("shaders/uioverlay.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT),
    };
    UIOverlay.prepareResources();
    UIOverlay.preparePipeline(pipelineCache, renderPass, swapChain.colorFormat,
                              depthFormat);
  }
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

void BaseRenderer::nextFrame() {
  auto tStart = std::chrono::high_resolution_clock::now();
  if (viewUpdated) {
    viewUpdated = false;
  }
  window->handleEvents();
  polledEvents(window->getWindow());
  render();
  frameCounter++;
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

  // TODO: Cap UI overlay update rates
  updateOverlay();
}

void BaseRenderer::renderLoop() {
  if (benchmark.active) {
    benchmark.run([=] { render(); }, vulkanDevice->properties);
    vkDeviceWaitIdle(device);
    if (benchmark.filename != "") {
      benchmark.saveResults();
    }
    return;
  }

  destWidth = width;
  destHeight = height;
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

void BaseRenderer::updateOverlay() {
  if (!settings.overlay) return;

  ImGuiIO& io = ImGui::GetIO();

  io.DisplaySize = ImVec2((float)width, (float)height);
  io.DeltaTime = frameTimer;

  io.MousePos = ImVec2(mouseState.position.x, mouseState.position.y);
  io.MouseDown[0] = mouseState.buttons.left && UIOverlay.visible;
  io.MouseDown[1] = mouseState.buttons.right && UIOverlay.visible;
  io.MouseDown[2] = mouseState.buttons.middle && UIOverlay.visible;

  ImGui::NewFrame();

  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
  ImGui::SetNextWindowPos(ImVec2(10 * UIOverlay.scale, 10 * UIOverlay.scale));
  ImGui::SetNextWindowSize(ImVec2(0, 0), ImGuiCond_FirstUseEver);
  ImGui::Begin("Vulkan Example", nullptr,
               ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize |
                   ImGuiWindowFlags_NoMove);
  ImGui::TextUnformatted(title.c_str());
  ImGui::TextUnformatted(deviceProperties.deviceName);
  ImGui::Text("%.2f ms/frame (%.1d fps)", (1000.0f / lastFPS), lastFPS);

  ImGui::PushItemWidth(110.0f * UIOverlay.scale);
  OnUpdateUIOverlay(&UIOverlay);
  ImGui::PopItemWidth();

  ImGui::End();
  ImGui::PopStyleVar();
  ImGui::Render();

  if (UIOverlay.update() || UIOverlay.updated) {
    buildCommandBuffers();
    UIOverlay.updated = false;
  }
}

void BaseRenderer::drawUI(const VkCommandBuffer commandBuffer) {
  if (settings.overlay && UIOverlay.visible) {
    const VkViewport viewport =
        vks::initializers::viewport((float)width, (float)height, 0.0f, 1.0f);
    const VkRect2D scissor = vks::initializers::rect2D(width, height, 0, 0);
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    UIOverlay.draw(commandBuffer);
  }
}

void BaseRenderer::prepareFrame() {
  // Acquire the next image from the swap chain
  VkResult result =
      swapChain.acquireNextImage(semaphores.presentComplete, &currentBuffer);
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
}

void BaseRenderer::submitFrame() {
  VkResult result =
      swapChain.queuePresent(queue, currentBuffer, semaphores.renderComplete);
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
  VK_CHECK_RESULT(vkQueueWaitIdle(queue));
}

BaseRenderer::BaseRenderer(std::vector<const char*> args) {
  struct stat info;
  if (stat(getAssetPath().c_str(), &info) != 0) {
    std::string msg =
        "Could not locate asset path in \"" + getAssetPath() + "\" !";
    MessageBox(NULL, msg.c_str(), "Fatal error", MB_OK | MB_ICONERROR);
    exit(-1);
  }

  // Validation for all samples can be forced at compile time using the
  // FORCE_VALIDATION define
#if defined(FORCE_VALIDATION)
  settings.validation = true;
#endif

  this->args = args;

  // Command line arguments
  commandLineParser.add("help", {"--help"}, 0, "Show help");
  commandLineParser.add("validation", {"-v", "--validation"}, 0,
                        "Enable validation layers");
  commandLineParser.add("vsync", {"-vs", "--vsync"}, 0, "Enable V-Sync");
  commandLineParser.add("fullscreen", {"-f", "--fullscreen"}, 0,
                        "Start in fullscreen mode");
  commandLineParser.add("width", {"-w", "--width"}, 1, "Set window width");
  commandLineParser.add("height", {"-h", "--height"}, 1, "Set window height");
  commandLineParser.add("shaders", {"-s", "--shaders"}, 1,
                        "Select shader type to use (glsl or hlsl)");
  commandLineParser.add("gpuselection", {"-g", "--gpu"}, 1,
                        "Select GPU to run on");
  commandLineParser.add("gpulist", {"-gl", "--listgpus"}, 0,
                        "Display a list of available Vulkan devices");
  commandLineParser.add("benchmark", {"-b", "--benchmark"}, 0,
                        "Run example in benchmark mode");
  commandLineParser.add("benchmarkwarmup", {"-bw", "--benchwarmup"}, 1,
                        "Set warmup time for benchmark mode in seconds");
  commandLineParser.add("benchmarkruntime", {"-br", "--benchruntime"}, 1,
                        "Set duration time for benchmark mode in seconds");
  commandLineParser.add("benchmarkresultfile", {"-bf", "--benchfilename"}, 1,
                        "Set file name for benchmark results");
  commandLineParser.add("benchmarkresultframes", {"-bt", "--benchframetimes"},
                        0, "Save frame times to benchmark results file");
  commandLineParser.add("benchmarkframes", {"-bfs", "--benchmarkframes"}, 1,
                        "Only render the given number of frames");

  commandLineParser.parse(args);
  if (commandLineParser.isSet("help")) {
    setupConsole("Vulkan example");
    commandLineParser.printHelp();
    std::cin.get();
    exit(0);
  }
  if (commandLineParser.isSet("validation")) {
    settings.validation = true;
  }
  if (commandLineParser.isSet("vsync")) {
    settings.vsync = true;
  }
  if (commandLineParser.isSet("height")) {
    height = commandLineParser.getValueAsInt("height", height);
  }
  if (commandLineParser.isSet("width")) {
    width = commandLineParser.getValueAsInt("width", width);
  }
  if (commandLineParser.isSet("fullscreen")) {
    settings.fullscreen = true;
  }
  if (commandLineParser.isSet("benchmark")) {
    benchmark.active = true;
    vks::tools::errorModeSilent = true;
  }
  if (commandLineParser.isSet("benchmarkwarmup")) {
    benchmark.warmup = commandLineParser.getValueAsInt("benchmarkwarmup", 0);
  }
  if (commandLineParser.isSet("benchmarkruntime")) {
    benchmark.duration =
        commandLineParser.getValueAsInt("benchmarkruntime", benchmark.duration);
  }
  if (commandLineParser.isSet("benchmarkresultfile")) {
    benchmark.filename = commandLineParser.getValueAsString(
        "benchmarkresultfile", benchmark.filename);
  }
  if (commandLineParser.isSet("benchmarkresultframes")) {
    benchmark.outputFrameTimes = true;
  }
  if (commandLineParser.isSet("benchmarkframes")) {
    benchmark.outputFrames = commandLineParser.getValueAsInt(
        "benchmarkframes", benchmark.outputFrames);
  }

  // Enable console if validation is active, debug message callback will output
  // to it
  if (this->settings.validation) {
    setupConsole("Vulkan example");
  }
}

BaseRenderer::~BaseRenderer() {
  // Clean up Vulkan resources
  swapChain.cleanup();
  if (descriptorPool != VK_NULL_HANDLE) {
    vkDestroyDescriptorPool(device, descriptorPool, nullptr);
  }
  destroyCommandBuffers();
  if (renderPass != VK_NULL_HANDLE) {
    vkDestroyRenderPass(device, renderPass, nullptr);
  }
  for (uint32_t i = 0; i < frameBuffers.size(); i++) {
    vkDestroyFramebuffer(device, frameBuffers[i], nullptr);
  }

  for (auto& shaderModule : shaderModules) {
    vkDestroyShaderModule(device, shaderModule, nullptr);
  }
  vkDestroyImageView(device, depthStencil.view, nullptr);
  vkDestroyImage(device, depthStencil.image, nullptr);
  vkFreeMemory(device, depthStencil.memory, nullptr);

  vkDestroyPipelineCache(device, pipelineCache, nullptr);

  vkDestroyCommandPool(device, cmdPool, nullptr);

  vkDestroySemaphore(device, semaphores.presentComplete, nullptr);
  vkDestroySemaphore(device, semaphores.renderComplete, nullptr);
  for (auto& fence : waitFences) {
    vkDestroyFence(device, fence, nullptr);
  }

  if (settings.overlay) {
    UIOverlay.freeResources();
  }

  delete vulkanDevice;

  if (settings.validation) {
    vks::debug::freeDebugCallback(instance);
  }

  vkDestroyInstance(instance, nullptr);
}

bool BaseRenderer::initVulkan() {
  VkResult err;

  // Vulkan instance
  err = createInstance(settings.validation);
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

  // GPU selection via command line argument
  if (commandLineParser.isSet("gpuselection")) {
    uint32_t index = commandLineParser.getValueAsInt("gpuselection", 0);
    if (index > gpuCount - 1) {
      std::cerr << "Selected device index " << index
                << " is out of range, reverting to device 0 (use -listgpus to "
                   "show available Vulkan devices)"
                << "\n";
    } else {
      selectedDevice = index;
    }
  }
  if (commandLineParser.isSet("gpulist")) {
    std::cout << "Available Vulkan devices"
              << "\n";
    for (uint32_t i = 0; i < gpuCount; i++) {
      VkPhysicalDeviceProperties deviceProperties;
      vkGetPhysicalDeviceProperties(physicalDevices[i], &deviceProperties);
      std::cout << "Device [" << i << "] : " << deviceProperties.deviceName
                << std::endl;
      std::cout << " Type: "
                << vks::tools::physicalDeviceTypeString(
                       deviceProperties.deviceType)
                << "\n";
      std::cout << " API: " << (deviceProperties.apiVersion >> 22) << "."
                << ((deviceProperties.apiVersion >> 12) & 0x3ff) << "."
                << (deviceProperties.apiVersion & 0xfff) << "\n";
    }
  }

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

  // Find a suitable depth and/or stencil format
  VkBool32 validFormat{false};
  // Samples that make use of stencil will require a depth + stencil format, so
  // we select from a different list
  if (requiresStencil) {
    validFormat = vks::tools::getSupportedDepthStencilFormat(physicalDevice,
                                                             &depthFormat);
  } else {
    validFormat =
        vks::tools::getSupportedDepthFormat(physicalDevice, &depthFormat);
  }
  assert(validFormat);

  swapChain.connect(instance, physicalDevice, device);

  // Create synchronization objects
  VkSemaphoreCreateInfo semaphoreCreateInfo =
      vks::initializers::semaphoreCreateInfo();
  // Create a semaphore used to synchronize image presentation
  // Ensures that the image is displayed before we start submitting new commands
  // to the queue
  VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr,
                                    &semaphores.presentComplete));
  // Create a semaphore used to synchronize command submission
  // Ensures that the image is not presented until all commands have been
  // submitted and executed
  VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr,
                                    &semaphores.renderComplete));

  // Set up submit info structure
  // Semaphores will stay the same during application lifetime
  // Command buffer submission info is set by each example
  submitInfo = vks::initializers::submitInfo();
  submitInfo.pWaitDstStageMask = &submitPipelineStages;
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = &semaphores.presentComplete;
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = &semaphores.renderComplete;

  return true;
}

// Win32 : Sets up a console window and redirects standard output to it
void BaseRenderer::setupConsole(std::string title) {
  AllocConsole();
  AttachConsole(GetCurrentProcessId());
  FILE* stream;
  freopen_s(&stream, "CONIN$", "r", stdin);
  freopen_s(&stream, "CONOUT$", "w+", stdout);
  freopen_s(&stream, "CONOUT$", "w+", stderr);
  // Enable flags so we can color the output
  HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
  DWORD dwMode = 0;
  GetConsoleMode(consoleHandle, &dwMode);
  dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
  SetConsoleMode(consoleHandle, dwMode);
  SetConsoleTitle(TEXT(title.c_str()));
}

void BaseRenderer::setupWindow() { window = new WindowManager(title.c_str()); }

//      if (camera.type == Camera::firstperson) {
//        switch (wParam) {
//
//        }
//      }
//
//      keyPressed((uint32_t)wParam);
//      break;
//    case WM_KEYUP:
//      if (camera.type == Camera::firstperson) {
//        switch (wParam) {
//          case KEY_W:
//            camera.keys.up = false;
//            break;
//          case KEY_S:
//            camera.keys.down = false;
//            break;
//          case KEY_A:
//            camera.keys.left = false;
//            break;
//          case KEY_D:
//            camera.keys.right = false;
//            break;
//        }
//      }
//      break;
//    case WM_LBUTTONDOWN:
//      mouseState.position =
//          glm::vec2((float)LOWORD(lParam), (float)HIWORD(lParam));
//      mouseState.buttons.left = true;
//      break;
//    case WM_RBUTTONDOWN:
//      mouseState.position =
//          glm::vec2((float)LOWORD(lParam), (float)HIWORD(lParam));
//      mouseState.buttons.right = true;
//      break;
//    case WM_MBUTTONDOWN:
//      mouseState.position =
//          glm::vec2((float)LOWORD(lParam), (float)HIWORD(lParam));
//      mouseState.buttons.middle = true;
//      break;
//
//    }
//    case WM_MOUSEMOVE: {
//      handleMouseMove(LOWORD(lParam), HIWORD(lParam));
//      break;
//    }
//    case WM_SIZE:
//      if ((prepared) && (wParam != SIZE_MINIMIZED)) {
//        if ((resizing) ||
//            ((wParam == SIZE_MAXIMIZED) || (wParam == SIZE_RESTORED))) {
//          destWidth = LOWORD(lParam);
//          destHeight = HIWORD(lParam);
//          windowResize();
//        }
//      }
//      break;
//    case WM_GETMINMAXINFO: {
//      LPMINMAXINFO minMaxInfo = (LPMINMAXINFO)lParam;
//      minMaxInfo->ptMinTrackSize.x = 64;
//      minMaxInfo->ptMinTrackSize.y = 64;
//      break;
//    }
//    case WM_ENTERSIZEMOVE:
//      resizing = true;
//      break;
//    case WM_EXITSIZEMOVE:
//      resizing = false;
//      break;

//    case WM_MOUSEWHEEL: {
//      short wheelDelta = GET_WHEEL_DELTA_WPARAM(wParam);
//      camera.translate(glm::vec3(0.0f, 0.0f, (float)wheelDelta * 0.005f));
//      viewUpdated = true;
//      break;
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
  if (glfwGetKey(window, GLFW_KEY_P)) {
    paused != paused;
  }
  if (glfwGetKey(window, GLFW_KEY_F1)) {
    UIOverlay.visible = !UIOverlay.visible;
    UIOverlay.updated = true;
  }
  if (glfwGetKey(window, GLFW_KEY_F2)) {
    if (camera.type == Camera::CameraType::lookat) {
      camera.type = Camera::CameraType::firstperson;
    } else {
      camera.type = Camera::CameraType::lookat;
    }
  }
  if (glfwGetKey(window, GLFW_KEY_ESCAPE)) {
    PostQuitMessage(0);
  }
  if (glfwGetKey(window, GLFW_KEY_W)) {
    camera.keys.up = true;
  } else {
    camera.keys.up = false;
  }
  if (glfwGetKey(window, GLFW_KEY_S)) {
    camera.keys.down = true;
  } else {
    camera.keys.down = false;
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
}

void BaseRenderer::buildCommandBuffers() {}

void BaseRenderer::createSynchronizationPrimitives() {
  // Wait fences to sync command buffer access
  VkFenceCreateInfo fenceCreateInfo =
      vks::initializers::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
  waitFences.resize(drawCmdBuffers.size());
  for (auto& fence : waitFences) {
    VK_CHECK_RESULT(vkCreateFence(device, &fenceCreateInfo, nullptr, &fence));
  }
}

void BaseRenderer::createCommandPool() {
  VkCommandPoolCreateInfo cmdPoolInfo = {};
  cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  cmdPoolInfo.queueFamilyIndex = swapChain.queueNodeIndex;
  cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  VK_CHECK_RESULT(vkCreateCommandPool(device, &cmdPoolInfo, nullptr, &cmdPool));
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

void BaseRenderer::setupFrameBuffer() {
  VkImageView attachments[2];

  // Depth/Stencil attachment is the same for all frame buffers
  attachments[1] = depthStencil.view;

  VkFramebufferCreateInfo frameBufferCreateInfo = {};
  frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  frameBufferCreateInfo.pNext = NULL;
  frameBufferCreateInfo.renderPass = renderPass;
  frameBufferCreateInfo.attachmentCount = 2;
  frameBufferCreateInfo.pAttachments = attachments;
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

void BaseRenderer::getEnabledFeatures() {}

void BaseRenderer::getEnabledExtensions() {}

void BaseRenderer::windowResize() {
  if (!prepared) {
    return;
  }
  prepared = false;
  resized = true;

  // Ensure all operations on the device have been finished before destroying
  // resources
  vkDeviceWaitIdle(device);

  // Recreate swap chain
  width = destWidth;
  height = destHeight;
  setupSwapChain();

  // Recreate the frame buffers
  vkDestroyImageView(device, depthStencil.view, nullptr);
  vkDestroyImage(device, depthStencil.image, nullptr);
  vkFreeMemory(device, depthStencil.memory, nullptr);
  setupDepthStencil();
  for (uint32_t i = 0; i < frameBuffers.size(); i++) {
    vkDestroyFramebuffer(device, frameBuffers[i], nullptr);
  }
  setupFrameBuffer();

  if ((width > 0.0f) && (height > 0.0f)) {
    if (settings.overlay) {
      UIOverlay.resize(width, height);
    }
  }

  // Command buffers need to be recreated as they may store
  // references to the recreated frame buffer
  destroyCommandBuffers();
  createCommandBuffers();
  buildCommandBuffers();

  // SRS - Recreate fences in case number of swapchain images has changed on
  // resize
  for (auto& fence : waitFences) {
    vkDestroyFence(device, fence, nullptr);
  }
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
  handled = io.WantCaptureMouse && UIOverlay.visible;

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

void BaseRenderer::windowResized() {}

void BaseRenderer::initSwapchain() {
  swapChain.initSurface(window->getWindow());
}

void BaseRenderer::setupSwapChain() {
  swapChain.create(&width, &height, settings.vsync, settings.fullscreen);
}

void BaseRenderer::OnUpdateUIOverlay(vks::UIOverlay* overlay) {}