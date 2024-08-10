#pragma once

#include <assert.h>

#include <algorithm>
#include <exception>

#if defined(_WIN32)
#define VK_USE_PLATFORM_WIN32_KHR
#elif defined(__linux__)
#define VK_USE_PLATFORM_XCB_KHR
#endif

#include <vk_mem_alloc.h>

#include "VulkanBuffer.h"
#include "VulkanTexture.h"
#include "VulkanTools.h"
#include "vulkan/vulkan.h"

namespace core_internal::rendering::vulkan {
struct SamplerInfo {
  VkFilter magFilter = VK_FILTER_LINEAR;
  VkFilter minFilter = VK_FILTER_LINEAR;
  VkSamplerMipmapMode mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  VkSamplerAddressMode addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  VkSamplerAddressMode addressModeV = samplerInfo.addressModeU;
  VkSamplerAddressMode addressModeW = samplerInfo.addressModeU;
  float mipLodBias = 0.0f;
  float maxAnisotropy = 1.0f;
  float minLod = 0.0f;
  float maxLod = 1.0f;
  VkBorderColor borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
};

struct ImageViewInfo {
  VkImageCreateFlags flags;
};

struct ImageInfo {
  uint32_t width;
  uint32_t height;
  VkFormat format;
  VkImageTiling tiling;
  VkImageUsageFlags usage;
  uint32_t mipLevels = 1;
  uint32_t arrayLayers = 1;
  uint32_t depth = 1;
  VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
  VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  bool requireSampler;
  bool requireImageView;
  bool requireMappedData;
  SamplerInfo samplerInfo;
  ImageViewInfo imageViewInfo;
};

struct Image {
  VkImage image = VK_NULL_HANDLE;
  VkImageLayout imageLayout;
  VmaAllocation deviceMemory = VK_NULL_HANDLE;
  VkImageView view = VK_NULL_HANDLE;
  VkSampler sampler = VK_NULL_HANDLE;
  VkDescriptorImageInfo descriptor;
  void *mappedData = nullptr;
};

struct Buffer {};

// Vulkan Resources are self contained in VulkanDevice
// VulkanDevice handles all resource creation/deletion
class VulkanDevice {
 public:
  uint32_t apiVersion = VK_API_VERSION_1_2;
  std::vector<std::string> supportedInstanceExtensions;
  std::vector<std::string> supportedExtensions;

  VkInstance instance;
  VkPhysicalDevice physicalDevice;
  VkDevice logicalDevice;
  VmaAllocator allocator;
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

  // Vulkan Resource Management
  // ---------------------------------------------------------------------------------------
  VkPipelineShaderStageCreateInfo loadShader(std::string fileName,
                                             VkShaderStageFlagBits stage);
  vks::Buffer *createBuffer(VkBufferUsageFlags usageFlags, VkDeviceSize size,
                            VkMemoryPropertyFlags memoryPropertyFlags,
                            uint32_t instanceCount);

  Image *createImage(const ImageInfo &imageCreateInfo,
                     const VkMemoryPropertyFlagBits &memoryFlags,
                     bool renderResource);
  std::vector<Image *> createAliasedImages(
      std::vector<ImageInfo> imageCreateInfos);
};
}  // namespace core_internal::rendering::vulkan