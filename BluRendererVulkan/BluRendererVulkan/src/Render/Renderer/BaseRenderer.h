#pragma once

#include <chrono>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>

#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL
#include <GLFW/glfw3native.h>
#include <imgui.h>

#include "../../Window/WindowManager.h"
#include "../ResourceManagement/VulkanResources/VulkanDevice.h"
#include "../ResourceManagement/VulkanResources/VulkanSwapchain.h"
#include "Camera/Camera.hpp"
#include "RenderGraph.hpp"

// Needs an interface with the engine to receive info
class BaseRenderer {
 private:
  // Window Info (Initially set upon swapchain creation)
  int width;
  int height;

  WindowManager* windowManager;
  core_internal::rendering::RenderGraph* renderGraph;

  // Debug
  uint32_t frameCounter = 0;
  uint32_t lastFPS = 0;
  std::chrono::time_point<std::chrono::high_resolution_clock> lastTimestamp,
      tPrevEnd;

  // Core
  void windowResize();
  void handleMouseMove(int32_t x, int32_t y);
  void polledEvents(GLFWwindow* window);

  // Entry point for the main render loop
  void renderLoop();

 protected:
  bool prepared = false;
  bool resized = false;
  bool viewUpdated = false;
  bool paused = false;

  std::string windowTitle = "Blu Renderer";
  std::string name = "Blu Renderer";

  std::vector<const char*> enabledDeviceExtensions;
  std::vector<const char*> enabledInstanceExtensions;

  struct Settings {
    // Activates validation layers (and message output) when set to true
    bool validation = true;
    // Set to true if fullscreen mode has been requested via command line
    bool fullscreen = true;
    // Set to true if v-sync will be forced for the swapchain
    bool vsync = false;
    // Enable UI overlay
    bool overlay = true;
  } settings;

  // TODO: M&K INPUT? MOVE INPUT TO ENGINE CLASS
  // State of mouse/touch input
  struct {
    struct {
      bool left = false;
      bool right = false;
      bool middle = false;
    } buttons;
    glm::vec2 position;
  } mouseState;

  // (Virtual)
  virtual void nextFrame();
  // (Pure virtual) Render function to be implemented by the sample
  // application
  virtual void render() = 0;
  // (Pure Virtual) Called when the window has been resized
  virtual void windowResized() = 0;
  //(Pure Virtual) Used to set features to enable on the device
  virtual void getEnabledFeatures() = 0;
  // (Pure Virtual) Used to enable extensions based on the supported extension
  // listing
  virtual void getEnabledExtensions() = 0;
  virtual void prepare();
  void prepareFrame();
  void submitFrame();

 public:
  core_internal::rendering::vulkan::VulkanDevice* vulkanDevice;
  core_internal::rendering::vulkan::VulkanSwapchain* vulkanSwapchain;

  Camera camera;
  float frameTimer = 1.0f;

  BaseRenderer();
  virtual ~BaseRenderer();

  void start();
  int getWidth();
  int getHeight();
};

// MOVE THIS
//  Defines a frame rate independent timer value clamped from -1.0...1.0
//  For use in animations, rotations, etc.
// float timer = 0.0f;
// Multiplier for speeding up (or slowing down) the global timer
// float timerSpeed = 0.25f;
// (Virtual) Move?
// virtual void createPipelineCache();
// MOVE TO DEVICE
// Loads a SPIR-V shader file for the given shader stage
// VkPipelineShaderStageCreateInfo loadShader(std::string fileName,
//                                            VkShaderStageFlagBits stage);