#include "../Window/WindowManager.h"

#include <stdexcept>

WindowManager::WindowManager(const char* name, int width, int height) {
  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE); 
  glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE); 
  window = glfwCreateWindow(width, height, name, nullptr, nullptr);

  // glfwSetWindowUserPointer(window, nullptr);

  glfwMakeContextCurrent(window);
   
  //glfwSwapInterval(1); //Vsync

}

WindowManager::~WindowManager() {
  glfwDestroyWindow(window);
  glfwTerminate();
}

void WindowManager::handleEvents() { glfwPollEvents(); }

GLFWwindow* WindowManager::getWindow() { return window; }