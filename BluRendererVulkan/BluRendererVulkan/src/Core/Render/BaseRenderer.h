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
#define GLFW_EXPOSE_NATIVE_WGL
#include <GLFW/glfw3native.h>
#include <imgui.h>

#include "../Engine/BaseEngine.h"
#include "../Window/WindowManager.h"
#include "RenderGraph.hpp"
#include "ResourceManagement/VulkanResources/VulkanDevice.h"
#include "ResourceManagement/VulkanResources/VulkanSwapchain.h"

// Needs an interface with the engine to receive info
class BaseRenderer {
 private:
  // Window Info (Initially set upon swapchain creation)
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
  core_internal::rendering::RenderGraph* renderGraph;

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
    glm::vec2 deltaPos;
  } mouseState;

  enum KeyBinds {
    KEYBOARD_Q = 1 << 0,
    KEYBOARD_W = 1 << 1,
    KEYBOARD_E = 1 << 2,
    KEYBOARD_R = 1 << 3,
    KEYBOARD_T = 1 << 4,
    KEYBOARD_Y = 1 << 5,
    KEYBOARD_U = 1 << 6,
    KEYBOARD_I = 1 << 7,
    KEYBOARD_O = 1 << 8,
    KEYBOARD_P = 1 << 9,
    KEYBOARD_A = 1 << 10,
    KEYBOARD_S = 1 << 11,
    KEYBOARD_D = 1 << 12,
    KEYBOARD_F = 1 << 13,
    KEYBOARD_G = 1 << 14,
    KEYBOARD_H = 1 << 15,
    KEYBOARD_J = 1 << 16,
    KEYBOARD_K = 1 << 17,
    KEYBOARD_L = 1 << 18,
    KEYBOARD_Z = 1 << 19,
    KEYBOARD_X = 1 << 20,
    KEYBOARD_C = 1 << 21,
    KEYBOARD_V = 1 << 22,
    KEYBOARD_B = 1 << 23,
    KEYBOARD_N = 1 << 24,
    KEYBOARD_M = 1 << 25,
    KEYBOARD_1 = 1 << 26,
    KEYBOARD_2 = 1 << 27,
    KEYBOARD_3 = 1 << 28,
    KEYBOARD_4 = 1 << 29,
    KEYBOARD_5 = 1 << 30,
    KEYBOARD_6 = 1 << 31,
    KEYBOARD_7 = 1 << 32,
    KEYBOARD_8 = 1 << 33,
    KEYBOARD_9 = 1 << 34,
    KEYBOARD_0 = 1 << 35,
    KEYBOARD_LSHIFT = 1 << 36,
    KEYBOARD_LCTRL = 1 << 37,
    KEYBOARD_LALT = 1 << 38,
    KEYBOARD_TAB = 1 << 39,
  };

  struct {
    unsigned long keys;
  } keyInput;

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
  // This still leaves Q
  // submission to the RenderGraph but maybe we could define it to the
  // rendergraph (I like this)
  virtual void render() = 0;

  virtual void prepare();

  void prepareFrame();
  void submitFrame();

 public:
  core_internal::rendering::vulkan::VulkanDevice* vulkanDevice;
  core_internal::rendering::vulkan::VulkanSwapchain* vulkanSwapchain;

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