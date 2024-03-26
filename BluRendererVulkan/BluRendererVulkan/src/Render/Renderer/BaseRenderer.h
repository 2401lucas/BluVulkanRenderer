#pragma once

#include <ShellScalingAPI.h>
#include <assert.h>
#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>
#include <windows.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <ctime>
#include <iostream>
#include <numeric>
#include <random>
#include <unordered_map>
#include <vector>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>

#include "../Debug/Benchmark.hpp"
#include "../Debug/CommandLineParser.hpp"
#include "../ParticleSystem/ParticleSystemHolder.h"
#include "../ResourceManagement/VulkanResources/SwapChain.h"
#include "../ResourceManagement/VulkanResources/VulkanDevice.h"
#include "Camera/Camera.hpp"
#include "UI/UIOverlay.h"

#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL
#define GLFW_EXPOSE_NATIVE_WGL
#include <GLFW/glfw3native.h>
#include <imgui.h>

#include "../../Window/WindowManager.h"

class BaseRenderer {
 private:
  std::string getWindowTitle();
  uint32_t destWidth;
  uint32_t destHeight;
  bool resizing = false;
  void handleMouseMove(int32_t x, int32_t y);
  void nextFrame();
  void updateOverlay();
  void createPipelineCache();
  void createCommandPool();
  void createSynchronizationPrimitives();
  void initSwapchain();
  void setupSwapChain();
  void createCommandBuffers();
  void destroyCommandBuffers();

 protected:
  uint32_t frameCounter = 0;
  uint32_t lastFPS = 0;
  std::chrono::time_point<std::chrono::high_resolution_clock> lastTimestamp,
      tPrevEnd;

  VkInstance instance{VK_NULL_HANDLE};
  std::vector<std::string> supportedInstanceExtensions;
  VkPhysicalDevice physicalDevice{VK_NULL_HANDLE};
  VkPhysicalDeviceProperties deviceProperties{};
  VkPhysicalDeviceFeatures deviceFeatures{};
  VkPhysicalDeviceMemoryProperties deviceMemoryProperties{};
  VkPhysicalDeviceFeatures enabledFeatures{};
  std::vector<const char*> enabledDeviceExtensions;
  std::vector<const char*> enabledInstanceExtensions;
  void* deviceCreatepNextChain = nullptr;
  VkDevice device{VK_NULL_HANDLE};
  VkQueue queue{VK_NULL_HANDLE};
  VkFormat depthFormat;
  VkCommandPool cmdPool{VK_NULL_HANDLE};
  VkPipelineStageFlags submitPipelineStages =
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  VkSubmitInfo submitInfo;
  std::vector<VkCommandBuffer> drawCmdBuffers;
  VkRenderPass renderPass{VK_NULL_HANDLE};
  std::vector<VkFramebuffer> frameBuffers;
  // Active frame buffer index
  uint32_t currentBuffer = 0;
  VkDescriptorPool descriptorPool{VK_NULL_HANDLE};
  std::vector<VkShaderModule> shaderModules;
  VkPipelineCache pipelineCache{VK_NULL_HANDLE};
  // Wraps the swap chain to present images (framebuffers) to the windowing
  // system
  SwapChain swapChain;
  // Synchronization semaphores
  struct {
    // Swap chain image presentation
    VkSemaphore presentComplete;
    // Command buffer submission and execution
    VkSemaphore renderComplete;
  } semaphores;
  std::vector<VkFence> waitFences;
  bool requiresStencil{false};

 public:
  bool prepared = false;
  bool resized = false;
  bool viewUpdated = false;
  uint32_t width = 1280;
  uint32_t height = 720;

  vks::UIOverlay UIOverlay;

  float frameTimer = 1.0f;

  vks::Benchmark benchmark;
  CommandLineParser commandLineParser;

  vks::VulkanDevice* vulkanDevice;

  struct Settings {
    /** @brief Activates validation layers (and message output) when set to true
     */
    bool validation = true;
    /** @brief Set to true if fullscreen mode has been requested via command
     * line */
    bool fullscreen = false;
    /** @brief Set to true if v-sync will be forced for the swapchain */
    bool vsync = false;
    /** @brief Enable UI overlay */
    bool overlay = true;
  } settings;

  /** @brief State of mouse/touch input */
  struct {
    struct {
      bool left = false;
      bool right = false;
      bool middle = false;
    } buttons;
    glm::vec2 position;
  } mouseState;

  VkClearColorValue defaultClearColor = {{0.025f, 0.025f, 0.025f, 1.0f}};

  std::vector<const char*> args;

  // Defines a frame rate independent timer value clamped from -1.0...1.0
  // For use in animations, rotations, etc.
  float timer = 0.0f;
  // Multiplier for speeding up (or slowing down) the global timer
  float timerSpeed = 0.25f;
  bool paused = false;

  Camera camera;

  std::string title = "Blu Renderer";
  std::string name = "test1";
  uint32_t apiVersion = VK_API_VERSION_1_0;

  /** @brief Default depth stencil attachment used by the default render pass */
  struct {
    VkImage image;
    VkDeviceMemory memory;
    VkImageView view;
  } depthStencil{};

  WindowManager* window;

  /** @brief Default base class constructor */
  BaseRenderer(std::vector<const char*> args);
  virtual ~BaseRenderer();
  /** @brief Setup the vulkan instance, enable required extensions and connect
   * to the physical device (GPU) */
  bool initVulkan();

  void setupConsole(std::string title);
  void setupWindow();

  /** @brief (Virtual) Creates the application wide Vulkan instance */
  virtual VkResult createInstance(bool enableValidation);
  /** @brief (Pure virtual) Render function to be implemented by the sample
   * application */
  virtual void render() = 0;
  /** @brief (Virtual) Called after window events have been polled, used
   * to do handle input*/
  virtual void polledEvents(GLFWwindow* window);
  /** @brief (Virtual) Called when the window has been resized, can be used by
   * the sample application to recreate resources */
  virtual void windowResized();
  /** @brief (Virtual) Called when resources have been recreated that require a
   * rebuild of the command buffers (e.g. frame buffer), to be implemented by
   * the sample application */
  virtual void buildCommandBuffers();
  /** @brief (Virtual) Setup default depth and stencil views */
  virtual void setupDepthStencil();
  /** @brief (Virtual) Setup default framebuffers for all requested swapchain
   * images */
  virtual void setupFrameBuffer();
  /** @brief (Virtual) Setup a default renderpass */
  virtual void setupRenderPass();
  /** @brief (Virtual) Called after the physical device features have been read,
   * can be used to set features to enable on the device */
  virtual void getEnabledFeatures();
  /** @brief (Virtual) Called after the physical device extensions have been
   * read, can be used to enable extensions based on the supported extension
   * listing*/
  virtual void getEnabledExtensions();

  /** @brief Prepares all Vulkan resources and functions required to run the
   * sample */
  virtual void prepare();

  /** @brief Loads a SPIR-V shader file for the given shader stage */
  VkPipelineShaderStageCreateInfo loadShader(std::string fileName,
                                             VkShaderStageFlagBits stage);

  void windowResize();

  /** @brief Entry point for the main render loop */
  void renderLoop();

  /** @brief Adds the drawing commands for the ImGui overlay to the given
   * command buffer */
  void drawUI(const VkCommandBuffer commandBuffer);

  /** Prepare the next frame for workload submission by acquiring the next swap
   * chain image */
  void prepareFrame();
  /** @brief Presents the current image to the swap chain */
  void submitFrame();
  /** @brief (Virtual) Default image acquire + submission and command buffer
   * submission function */
  virtual void renderFrame();

  /** @brief (Virtual) Called when the UI overlay is updating, can be used to
   * add custom elements to the overlay */
  virtual void OnUpdateUIOverlay(vks::UIOverlay* overlay);
};