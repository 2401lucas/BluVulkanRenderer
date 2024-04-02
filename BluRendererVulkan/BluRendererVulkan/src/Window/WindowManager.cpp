#include "../Window/WindowManager.h"

#include <stdexcept>

WindowManager::WindowManager(const char* name, int width, int height) {
  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  window = glfwCreateWindow(width, height, name, nullptr, nullptr);

  // glfwSetWindowUserPointer(window, nullptr);

  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);

  // TODO: 6 CALLBACK FOR WINDOW RESIZE & MINIMIZING
  // glfwSetFramebufferSizeCallback();
}

WindowManager::~WindowManager() {
  glfwDestroyWindow(window);
  glfwTerminate();
}

void WindowManager::handleEvents() { glfwPollEvents(); }

GLFWwindow* WindowManager::getWindow() { return window; }