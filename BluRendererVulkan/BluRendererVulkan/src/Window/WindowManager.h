#pragma once
#define GLFW_INCLUDE_VULKAN
#define GLFW_EXPOSE_NATIVE_WIN32

#include <GLFW/glfw3.h>

class WindowManager {
 public:
  WindowManager(const char* name, int* width, int* height, bool fullscreen);
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