#ifndef WINDOW_H
#define WINDOW_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace blu::core {
class Window {
 public:
  Window();
  ~Window();

  GLFWwindow* Get() const { return window_; };
  uint32_t GetWidth() const { return width_; }
  uint32_t GetHeight() const { return height_; }

 private:
  GLFWwindow* window_;
  uint32_t width_;
  uint32_t height_;
};
}  // namespace blu::core
#endif