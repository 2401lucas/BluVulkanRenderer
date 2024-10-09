#pragma once

#ifdef DEBUG_ALL
#define DEBUG_RENDERER
#define DEBUG_RENDERGRAPH
#define DEBUG_ENGINE
#endif

#include <chrono>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <imgui.h>

#include "../Engine/BaseEngine.h"
#include "../Window/WindowManager.h"
#include "Components/Input.hpp"
#include "RenderGraph.hpp"
#include "ResourceManagement/VulkanResources/VulkanDevice.h"
#include "ResourceManagement/VulkanResources/VulkanSwapchain.h"

// Needs to interface with the engine to receive info
class BaseRenderer {
 private:
  // Window Info
  int width;
  int height;

  WindowManager* windowManager;

  uint32_t frameCounter = 0;
  uint32_t lastFPS = 0;
  std::chrono::time_point<std::chrono::high_resolution_clock> lastTimestamp,
      tPrevEnd;
  float deltaTime = 0;

  // Core
  void windowResize();
  void handleMouseMove(int32_t x, int32_t y);
  void handleMousepress(GLFWwindow* window, int glfwKey, unsigned long keyCode,
                        unsigned long prevInput);
  void handleKeypress(GLFWwindow* window, int glfwKey, unsigned long keyCode,
                      const unsigned long& prevInput);
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

  void* pNextChain = nullptr;
  std::vector<const char*> enabledDeviceExtensions;
  std::vector<const char*> enabledInstanceExtensions;

  core_internal::engine::BaseEngine* engine;
  core_internal::rendering::rendergraph::RenderGraph* renderGraph;

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

  InputData input;

  BaseRenderer();
  virtual ~BaseRenderer();

  // (Virtual)
  virtual void nextFrame();
  // (Pure Virtual) Called when the window has been resized
  virtual void windowResized() = 0;
  //(Pure Virtual) Used to request features
  virtual void getEnabledFeatures() = 0;
  // (Pure Virtual) Used to request extensions
  virtual void getEnabledExtensions() = 0;
  // (Pure virtual)
  virtual void buildEngine() = 0;
  // (Pure virtual) Called every frame? maybe should be considered pre-render
  // for updating buffers and such. I think a flow of
  // Engine::CollectRenderData->Renderer::PreRenderUpdateData->BaseRenderer::StartRenderGraph
  // Need to keep in mind how to multi-thread and sync work. I do not know.
  // Unless:
  // Engine updates data to pointer in mem, then renderer locks & reads
  // data while engine builds next frame to update with another pointer. Once
  // renderer is done reading & unlocks mem, engine can lock, copy and unlock
  // mem.
  // OR:
  // Ping pong between memory pointers (This is the way, no memory
  // copying and allows for engine to work) In this case, multiple threads can
  // read from the same pointer(assuming no writes) which can be defined with
  // https://en.wikipedia.org/wiki/Immutable_object
  // This still leaves Queue submission to the RenderGraph but maybe we could
  // define it to the rendergraph (I like this)
  virtual void render() = 0;

  virtual void prepare() = 0;

  void prepareFrame();
  void submitFrame();

 public:
  core_internal::rendering::VulkanDevice* vulkanDevice;
  core_internal::rendering::VulkanSwapchain* vulkanSwapchain;

  float frameTimer = 1.0f;

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