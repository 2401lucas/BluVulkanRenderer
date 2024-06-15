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

#include "../ResourceManagement/VulkanResources/SwapChain.h"
#include "../ResourceManagement/VulkanResources/VulkanDevice.h"
#include "Camera/Camera.hpp"

#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL
#include <GLFW/glfw3native.h>
#include <imgui.h>

#include "../../Window/WindowManager.h"

class BaseRenderer {
 private:
  // Window Info
  std::string title = "Blu Renderer: ";
  uint32_t apiVersion = VK_API_VERSION_1_0;
  uint32_t width = 1920;
  uint32_t height = 1080;

  WindowManager* window;

  // Rendering Settings
  VkSampleCountFlagBits msaaSampleCount = VK_SAMPLE_COUNT_1_BIT;

  // Debug
  uint32_t lastFPS = 0;
  std::chrono::time_point<std::chrono::high_resolution_clock> lastTimestamp,
      tPrevEnd;

  // Core
  void createCommandPools();
  void createSynchronizationPrimitives();
  void destroySynchronizationPrimitives();
  void initSwapchain();
  void setupSwapChain();
  bool initVulkan();
  VkPhysicalDevice choosePhysicalDevice(std::vector<VkPhysicalDevice>);
  int rateDeviceSuitability(VkPhysicalDevice);
  void setupWindow();
  void windowResize();
  VkResult createInstance();
  void handleMouseMove(int32_t x, int32_t y);

  // Entry point for the main render loop
  void renderLoop();

 protected:
  bool prepared = false;
  bool resized = false;
  bool viewUpdated = false;

  std::string name = "Blu Renderer";
  uint32_t frameCounter = 0;
  // Defines a frame rate independent timer value clamped from -1.0...1.0
  // For use in animations, rotations, etc.
  float timer = 0.0f;
  // Multiplier for speeding up (or slowing down) the global timer
  float timerSpeed = 0.25f;
  bool paused = false;

  // Vulkan Resources
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
  VkQueue graphicsQueue{VK_NULL_HANDLE};
  VkQueue computeQueue{VK_NULL_HANDLE};
  VkFormat depthFormat;
  VkCommandPool graphicsCmdPool{VK_NULL_HANDLE};
  VkCommandPool computeCmdPool{VK_NULL_HANDLE};
  VkPipelineStageFlags submitPipelineStages =
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  std::vector<VkCommandBuffer> drawCmdBuffers;
  std::vector<VkCommandBuffer> computeCmdBuffers;
  std::vector<VkFramebuffer> frameBuffers;
  uint32_t currentFrameIndex = 0;
  uint32_t currentImageIndex = 0;
  VkDescriptorPool descriptorPool{VK_NULL_HANDLE};
  std::vector<VkShaderModule> shaderModules;
  // Simple Pipeline Cache which can be shared among simple that share the
  // majority of their state, but more complex graphics pipelines should get
  // their own
  VkPipelineCache pipelineCache{VK_NULL_HANDLE};

  struct {
    VkImage image{VK_NULL_HANDLE};
    VkImageView view{VK_NULL_HANDLE};
    VkDeviceMemory memory{VK_NULL_HANDLE};
  } depthStencil;

  // Synchronization semaphores
  struct {
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
  } semaphores;

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
  // (Virtual)
  virtual void createPipelineCache();
  // (Virtual)
  virtual void createCommandBuffers();
  // (Virtual)
  virtual void destroyCommandBuffers();
  // (Pure virtual) Render function to be implemented by the sample
  // application
  virtual void render() = 0;
  //(Virtual) Called after window events have been polled, used
  // to do handle input
  virtual void polledEvents(GLFWwindow* window);
  // (Virtual) Called when the window has been resized
  virtual void windowResized();
  virtual void buildCommandBuffer();
  //(Virtual) Setup default framebuffers for all requested swapchain images
  virtual void setupFrameBuffer();
  // (Virtual) Setup a default renderpass
  virtual void setupRenderPass();
  // (Virtual) Setup a default Depth Stencil
  virtual void setupDepthStencil();
  //(Virtual) Called after the physical device features have been read,
  //  can be used to set features to enable on the device
  virtual void getEnabledFeatures();
  // (Virtual) Called after the physical device extensions have been
  // read, can be used to enable extensions based on the supported extension
  // listing
  virtual void getEnabledExtensions();
  // Prepares all Vulkan resources and functions required to run the
  virtual void prepare();

  // Prepare the next frame for workload submission by acquiring the next swap
  // chain image
  void prepareFrame();
  // Presents the current image to the swap chain
  void submitFrame();
  void setSampleCount(VkSampleCountFlagBits);

 public:
  BaseRenderer();
  virtual ~BaseRenderer();
  VkSampleCountFlagBits getMSAASampleCount();
  // Loads a SPIR-V shader file for the given shader stage
  VkPipelineShaderStageCreateInfo loadShader(std::string fileName,
                                             VkShaderStageFlagBits stage);
  void start();
  const char* getTitle();
  uint32_t getWidth();
  uint32_t getHeight();

  float frameTimer = 1.0f;

  vks::VulkanDevice* vulkanDevice;
  SwapChain swapChain;
  Camera camera;
  VkRenderPass renderPass{VK_NULL_HANDLE};
};