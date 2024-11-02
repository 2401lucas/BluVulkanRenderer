#include "Window.h"

#include <iostream>

namespace blu::core {
Window::Window(int width, int height, const char* title) {
  if (!glfwInit()) {
    std::cerr << "GLFW failed to initialize";
    return;
  }
  window_ = glfwCreateWindow(width, height, title, nullptr, nullptr);
  if (window_ == NULL) {
    glfwTerminate();
    std::cerr << "GLFW Window failed to be initialized";
    return;
  }

  glfwMakeContextCurrent(window_);
}

Window::~Window() {
  glfwDestroyWindow(window_);
  glfwTerminate();
}

bool Window::ShouldClose() { return glfwWindowShouldClose(window_); }

void Window::ProcessEvents() { glfwPollEvents(); }
}  // namespace blu::core
