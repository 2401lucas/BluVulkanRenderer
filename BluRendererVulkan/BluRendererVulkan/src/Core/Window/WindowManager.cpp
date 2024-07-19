#include "../Window/WindowManager.h"

#include <stdexcept>

WindowManager::WindowManager(const char* name, int* width, int* height,
                             bool fullscreen) {
  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);
  glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);
  window = glfwCreateWindow(1280, 720, name, nullptr, nullptr);
  glfwGetFramebufferSize(window, width, height);

  // glfwSetWindowUserPointer(window, nullptr);
  glfwMakeContextCurrent(window);
}

WindowManager::~WindowManager() {
  glfwDestroyWindow(window);
  glfwTerminate();
}

void WindowManager::handleEvents() { glfwPollEvents(); }

GLFWwindow* WindowManager::getWindow() { return window; }