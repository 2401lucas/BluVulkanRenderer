#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class WindowManager {
 public:
  WindowManager(const char* name, int width, int height);
  ~WindowManager();

  void handleEvents();
  GLFWwindow* getWindow();

  enum class CursorMode {
    Normal,
    Hidden,
    Disabled,
  };

 private:
  GLFWwindow* window;
};