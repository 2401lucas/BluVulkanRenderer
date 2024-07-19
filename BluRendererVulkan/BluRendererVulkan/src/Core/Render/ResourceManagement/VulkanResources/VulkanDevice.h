#pragma once

#include <assert.h>

#include <algorithm>
#include <exception>

#if defined(_WIN32)
#define VK_USE_PLATFORM_WIN32_KHR
#elif defined(__linux__)
#define VK_USE_PLATFORM_XCB_KHR
#endif

#include "VulkanBuffer.h"
#include "VulkanTools.h"
#include "vulkan/vulkan.h"

namespace core_internal::rendering::vulkan {
class VulkanDevice {
 public:
  uint32_t apiVersion = VK_API_VERSION_1_0;
  std::vector<std::string> supportedInstanceExtensions;
  std::vector<std::string> supportedExtensions;

  VkInstance instance;
  VkPhysicalDevice physicalDevice;
  VkDevice logicalDevice;
  VkPhysicalDeviceProperties physicalDeviceProperties;
  VkPhysicalDeviceFeatures physicalDeviceFeatures;
  VkPhysicalDeviceFeatures physicalDeviceEnabledFeatures;
  VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;
  std::vector<VkQueueFamilyProperties> queueFamilyProperties;
  std::vector<VkShaderModule> shaderModules;

  struct {
    uint32_t graphics;
    uint32_t compute;
    uint32_t transfer;
  } queueFamilyIndices;

  explicit VulkanDevice(const char *name, bool useValidation,
                        std::vector<const char *> enabledDeviceExtensions,
                        std::vector<const char *> enabledInstanceExtensions,
                        void *pNextChain = nullptr);
  ~VulkanDevice();

  // Helper---------------------------------------------------------------------------------------
  uint32_t getMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties,
                         VkBool32 *memTypeFound = nullptr) const;
  uint32_t getQueueFamilyIndex(VkQueueFlags queueFlags) const;
  VkPhysicalDevice choosePhysicalDevice(std::vector<VkPhysicalDevice> devices);
  int rateDeviceSuitability(VkPhysicalDevice device);
  bool extensionSupported(std::string extension);
  VkFormat getSupportedDepthFormat(bool checkSamplingSupport);
  void waitIdle();

  // Vulkan_Resource_Management---------------------------------------------------------------------------------------
  VkPipelineShaderStageCreateInfo loadShader(std::string fileName,
                                             VkShaderStageFlagBits stage);

  VkResult createBuffer(VkBufferUsageFlags usageFlags,
                        VkMemoryPropertyFlags memoryPropertyFlags,
                        VkDeviceSize size, VkBuffer *buffer,
                        VkDeviceMemory *memory, void *data = nullptr);
  VkResult createBuffer(VkBufferUsageFlags usageFlags,
                        VkMemoryPropertyFlags memoryPropertyFlags,
                        vks::Buffer *buffer, VkDeviceSize size,
                        void *data = nullptr);
};
}  // namespace core_internal::rendering::vulkan