#include "BaseRenderer.h"

#include <map>

BaseRenderer::BaseRenderer() {
  windowManager = new WindowManager(windowTitle.c_str(), &width, &height,
                                    settings.fullscreen);
  getEnabledFeatures();
  getEnabledExtensions();

  vulkanDevice = new core_internal::rendering::vulkan::VulkanDevice(
      windowTitle.c_str(), settings.validation, enabledDeviceExtensions,
      enabledInstanceExtensions, pNextChain);
  vulkanSwapchain = new core_internal::rendering::vulkan::VulkanSwapchain(
      vulkanDevice, windowManager->getWindow());
  vulkanSwapchain->create(&width, &height, settings.vsync, settings.fullscreen);
  renderGraph = new core_internal::rendering::RenderGraph();
  buildEngine();
}

BaseRenderer::~BaseRenderer() {
  delete vulkanSwapchain;
  delete renderGraph;
  delete vulkanDevice;
}

void BaseRenderer::start() { renderLoop(); }

// IDK------------------------------------------------------------------------------------------------------------
// void BaseRenderer::createPipelineCache() {
//  VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
//  pipelineCacheCreateInfo.sType =
//  VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
//  VK_CHECK_RESULT(vkCreatePipelineCache(device, &pipelineCacheCreateInfo,
//                                        nullptr, &pipelineCache));
//}

// IDK------------------------------------------------------------------------------------------------------------
// void BaseRenderer::setupFrameBuffer() {
//  std::array<VkImageView, 2> attachments;
//  // attachment[0]
//  // Depth/Stencil attachment is the same for all frame buffers
//  attachments[1] = depthStencil.view;
//
//  VkFramebufferCreateInfo frameBufferCreateInfo = {};
//  frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
//  frameBufferCreateInfo.pNext = NULL;
//  frameBufferCreateInfo.renderPass = renderPass;
//  frameBufferCreateInfo.attachmentCount =
//      static_cast<uint32_t>(attachments.size());
//  frameBufferCreateInfo.pAttachments = attachments.data();
//  frameBufferCreateInfo.width = width;
//  frameBufferCreateInfo.height = height;
//  frameBufferCreateInfo.layers = 1;
//
//  // Create frame buffers for every swap chain image
//  frameBuffers.resize(swapChain.imageCount);
//  for (uint32_t i = 0; i < frameBuffers.size(); i++) {
//    attachments[0] = swapChain.buffers[i].view;
//    VK_CHECK_RESULT(vkCreateFramebuffer(device, &frameBufferCreateInfo,
//    nullptr,
//                                        &frameBuffers[i]));
//  }
//}

void BaseRenderer::nextFrame() {
  auto tStart = std::chrono::high_resolution_clock::now();

  // engine->beginFixedUpdate(deltaTime);
  engine->beginUpdate(deltaTime);
  windowManager->handleEvents();
  polledEvents(windowManager->getWindow());
  render();

  frameCounter++;
  // currentFrameIndex = (currentFrameIndex + 1) % vulkanSwapchain->imageCount;
  auto tEnd = std::chrono::high_resolution_clock::now();
  deltaTime = std::chrono::duration<double, std::milli>(tEnd - tStart).count();

  frameTimer = (float)deltaTime / 1000.0f;

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
  vulkanDevice->waitIdle();
}

void BaseRenderer::prepareFrame() { renderGraph->prepareFrame(); }
// Instead of submit, this should be validate frame or something, leave Q
// dispatching to inherited class
void BaseRenderer::submitFrame() {
  // Needs multiple Q submits, maybe just verify their outputs and dispatch Q
  // submits elsewhere
  VkResult result = renderGraph->submitFrame();
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

void BaseRenderer::handleKeypress(GLFWwindow* window, int glfwKey,
                                  unsigned long keyCode,
                                  unsigned long prevInput) {
  if (glfwGetKey(window, glfwKey)) {
    if ((prevInput & keyCode) == 0) keyInput.isFirstFrame |= keyCode;
    keyInput.isHeld |= 1 << keyCode;
  } else {
    if ((prevInput & keyCode) == 1) {
      keyInput.isReleased |= keyCode;
    }
  }
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

  if (glfwGetKey(window, GLFW_KEY_ESCAPE) || glfwWindowShouldClose(window)) {
    PostQuitMessage(0);
  }

  ImGuiIO& io = ImGui::GetIO();

  if (io.WantCaptureKeyboard && settings.overlay) {
    keyInput.isHeld = 0;
    keyInput.isFirstFrame = 0;
    keyInput.isReleased = 0;
    return;
  }
  unsigned long prevInput = keyInput.isHeld;

  if (glfwGetKey(window, GLFW_KEY_P)) {
    paused != paused;
  }

  handleKeypress(window, GLFW_KEY_W, KEYBOARD_W, prevInput);
  handleKeypress(window, GLFW_KEY_S, KEYBOARD_S, prevInput);
  handleKeypress(window, GLFW_KEY_A, KEYBOARD_A, prevInput);
  handleKeypress(window, GLFW_KEY_D, KEYBOARD_D, prevInput);
  handleKeypress(window, GLFW_KEY_LEFT_CONTROL, KEYBOARD_LCTRL, prevInput);
  handleKeypress(window, GLFW_KEY_LEFT_SHIFT, KEYBOARD_LSHIFT, prevInput);
}

void BaseRenderer::windowResize() {
  if (!prepared) {
    return;
  }
  prepared = false;
  resized = true;

  vulkanDevice->waitIdle();

  // Recreate swap chain
  int destWidth, destHeight;
  glfwGetFramebufferSize(windowManager->getWindow(), &destWidth, &destHeight);
  while (destWidth == 0 || destHeight == 0) {
    glfwGetFramebufferSize(windowManager->getWindow(), &destWidth, &destHeight);
    glfwWaitEvents();
  }

  width = destWidth;
  height = destHeight;

  vulkanSwapchain->create(&width, &height, settings.vsync, settings.fullscreen);

  if ((width > 0.0f) && (height > 0.0f)) {
    engine->onResized((float)width / (float)height);
    renderGraph->onResized();
  }

  windowResized();

  prepared = true;
}

void BaseRenderer::handleMouseMove(int32_t x, int32_t y) {
  ImGuiIO& io = ImGui::GetIO();

  if (io.WantCaptureMouse && settings.overlay) {
    mouseState.position = glm::vec2((float)x, (float)y);
    mouseState.deltaPos = glm::vec2(0, 0);
    return;
  }

  mouseState.deltaPos = {mouseState.position.x - x, mouseState.position.y - y};
}

int BaseRenderer::getWidth() { return width; }

int BaseRenderer::getHeight() { return height; }