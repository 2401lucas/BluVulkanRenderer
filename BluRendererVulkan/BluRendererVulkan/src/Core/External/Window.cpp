#include "Window.h"

#include <iostream>

namespace blu::core {
Window::Window(int width, int height, const char* title) {
  if (!glfwInit()) {
    std::cerr << "GLFW failed to initialize";
    return;
  }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_MAXIMIZED, GLFW_FALSE);
  glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_FALSE);

  window_ = glfwCreateWindow(width, height, title, nullptr, nullptr);
  width_ = width;
  height_ = height;
  if (window_ == NULL) {
    glfwTerminate();
    std::cerr << "GLFW Window failed to be initialized";
    return;
  }
  // glfwSetWindowTitle();
  glfwMakeContextCurrent(window_);
  glfwSetErrorCallback(ErrorMsg);
}

void Window::ErrorMsg(int error_code, const char* description) {
  std::cerr << error_code << description;
}

Window::~Window() {
  glfwDestroyWindow(window_);
  glfwTerminate();
}

bool Window::ShouldClose() { return glfwWindowShouldClose(window_); }

void Window::ProcessEvents() { glfwPollEvents(); }
}  // namespace blu::core
