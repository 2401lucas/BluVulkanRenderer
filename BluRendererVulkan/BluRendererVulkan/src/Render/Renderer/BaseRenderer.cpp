#include "BaseRenderer.h"

#include <map>

BaseRenderer::BaseRenderer() {
  windowManager = new WindowManager(windowTitle.c_str(), &width, &height,
                                    settings.fullscreen);
  getEnabledFeatures();
  getEnabledExtensions();

  vulkanDevice = new core_internal::rendering::vulkan::VulkanDevice(
      windowTitle.c_str(), settings.validation, enabledDeviceExtensions,
      enabledInstanceExtensions);
  vulkanSwapchain = new core_internal::rendering::vulkan::VulkanSwapchain();
  renderGraph = new core_internal::rendering::RenderGraph();
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
  if (viewUpdated) {
    viewUpdated = false;
  }
  windowManager->handleEvents();
  polledEvents(windowManager->getWindow());
  render();

  frameCounter++;
  // currentFrameIndex = (currentFrameIndex + 1) % vulkanSwapchain->imageCount;
  auto tEnd = std::chrono::high_resolution_clock::now();
  auto tDiff = std::chrono::duration<double, std::milli>(tEnd - tStart).count();

  frameTimer = (float)tDiff / 1000.0f;
  camera.update(frameTimer);
  if (camera.moving()) {
    viewUpdated = true;
  }
  // Convert to clamped timer value
  // if (!paused) {
  // timer += timerSpeed * frameTimer;
  // if (timer > 1.0) {
  //   timer -= 1.0f;
  // }
  // }
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

void BaseRenderer::submitFrame() {
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
// TODO: Build input struct to send around
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

int BaseRenderer::getWidth() { return width; }

int BaseRenderer::getHeight() { return height; }