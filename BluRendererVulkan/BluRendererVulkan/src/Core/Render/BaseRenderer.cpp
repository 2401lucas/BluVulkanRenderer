#include "BaseRenderer.h"

#include <map>

BaseRenderer::BaseRenderer() {
  windowManager = new WindowManager(windowTitle.c_str(), &width, &height,
                                    settings.fullscreen);
  getEnabledFeatures();
  getEnabledExtensions();

  vulkanDevice = new core_internal::rendering::VulkanDevice(
      windowTitle.c_str(), settings.validation, enabledDeviceExtensions,
      enabledInstanceExtensions, pNextChain);
  vulkanSwapchain = new core_internal::rendering::VulkanSwapchain(
      vulkanDevice, windowManager->getWindow());
  vulkanSwapchain->create(&width, &height, settings.vsync, settings.fullscreen);
  renderGraph =
      new core_internal::rendering::rendergraph::RenderGraph({width, height});
  buildEngine();
}

BaseRenderer::~BaseRenderer() {
  delete renderGraph;
  delete vulkanSwapchain;
  delete vulkanDevice;
}

void BaseRenderer::start() { renderLoop(); }

void BaseRenderer::nextFrame() {
  auto tStart = std::chrono::high_resolution_clock::now();

  // engine->beginFixedUpdate(deltaTime);
  engine->beginUpdate(deltaTime, input);
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

void BaseRenderer::prepareFrame() { /*renderGraph->prepareFrame();*/ }
// Instead of submit, this should be validate frame or something, leave Q
// dispatching to inherited class
void BaseRenderer::submitFrame() {
  // Needs multiple Q submits, maybe just verify their outputs and dispatch Q
  // submits elsewhere
  //VkResult result = renderGraph->submitFrame();
  // Recreate the swapchain if it's no longer compatible with the surface
  //// (OUT_OF_DATE) or no longer optimal for presentation (SUBOPTIMAL)
  //if ((result == VK_ERROR_OUT_OF_DATE_KHR) || (result == VK_SUBOPTIMAL_KHR)) {
  //  windowResize();
  //  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
  //    return;
  //  }
  //} else {
  //  VK_CHECK_RESULT(result);
  //}
}

void BaseRenderer::handleMousepress(GLFWwindow* window, int glfwKey,
                                    unsigned long keyCode,
                                    unsigned long prevInput) {
  if (glfwGetMouseButton(window, glfwKey)) {
    if ((prevInput & keyCode) == 0) input.isFirstFrame |= keyCode;
    input.isPressed |= 1 << keyCode;
  } else {
    if ((prevInput & keyCode) == 1) {
      input.isReleased |= keyCode;
    }
  }
}

void BaseRenderer::handleKeypress(GLFWwindow* window, int glfwKey,
                                  unsigned long keyCode,
                                  const unsigned long& prevInput) {
  if (glfwGetKey(window, glfwKey)) {
    if ((prevInput & keyCode) == 0) input.isFirstFrame |= keyCode;
    input.isPressed |= 1 << keyCode;
  } else {
    if ((prevInput & keyCode) == 1) {
      input.isReleased |= keyCode;
    }
  }
}

void BaseRenderer::polledEvents(GLFWwindow* window) {
  double xpos, ypos;
  glfwGetCursorPos(window, &xpos, &ypos);
  unsigned long prevInput = input.isPressed;

  handleMousepress(window, GLFW_MOUSE_BUTTON_LEFT,
                   InputData::KeyBinds::MOUSE_LEFT, prevInput);
  handleMousepress(window, GLFW_MOUSE_BUTTON_RIGHT,
                   InputData::KeyBinds::MOUSE_RIGHT, prevInput);
  handleMouseMove(xpos, ypos);

  if (glfwGetKey(window, GLFW_KEY_ESCAPE) || glfwWindowShouldClose(window)) {
    PostQuitMessage(0);
  }

  ImGuiIO& io = ImGui::GetIO();

  if (io.WantCaptureKeyboard && settings.overlay) {
    input.isPressed = 0;
    input.isFirstFrame = 0;
    input.isReleased = 0;
    return;
  }

  if (glfwGetKey(window, GLFW_KEY_P)) {
    paused != paused;
  }

  handleKeypress(window, GLFW_KEY_W, InputData::KeyBinds::KEYBOARD_W,
                 prevInput);
  handleKeypress(window, GLFW_KEY_S, InputData::KeyBinds::KEYBOARD_S,
                 prevInput);
  handleKeypress(window, GLFW_KEY_A, InputData::KeyBinds::KEYBOARD_A,
                 prevInput);
  handleKeypress(window, GLFW_KEY_D, InputData::KeyBinds::KEYBOARD_D,
                 prevInput);
  handleKeypress(window, GLFW_KEY_LEFT_CONTROL,
                 InputData::KeyBinds::KEYBOARD_LCTRL, prevInput);
  handleKeypress(window, GLFW_KEY_LEFT_SHIFT,
                 InputData::KeyBinds::KEYBOARD_LSHIFT, prevInput);
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
    //renderGraph->onResized();
  }

  windowResized();

  prepared = true;
}

void BaseRenderer::handleMouseMove(int32_t x, int32_t y) {
  ImGuiIO& io = ImGui::GetIO();

  if (io.WantCaptureMouse && settings.overlay) {
    input.position = glm::vec2((float)x, (float)y);
    input.deltaPos = glm::vec2(0, 0);
    return;
  }

  input.deltaPos = {input.position.x - x, input.position.y - y};
  input.position = glm::vec2((float)x, (float)y);
}

int BaseRenderer::getWidth() { return width; }

int BaseRenderer::getHeight() { return height; }