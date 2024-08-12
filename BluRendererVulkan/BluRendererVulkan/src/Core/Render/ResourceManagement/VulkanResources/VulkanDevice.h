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
  VkSamplerAddressMode addressModeV = addressModeU;
  VkSamplerAddressMode addressModeW = addressModeU;
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
  VkDeviceSize offset = 0;
  // The only limitaion to having more than 32 dependency layers is this
  unsigned long resourceLifespan;
};

struct Image {
  VkImage image = VK_NULL_HANDLE;
  VkImageLayout imageLayout;
  VmaAllocation deviceMemory = VK_NULL_HANDLE;
  VkDeviceSize offset = 0;
  VkImageView view = VK_NULL_HANDLE;
  VkSampler sampler = VK_NULL_HANDLE;
  VkDescriptorImageInfo descriptor;
  void *mappedData = nullptr;
};

struct BufferInfo {
  VkDeviceSize size;
  VkDeviceSize offset;
  VkBufferUsageFlags usage;
  bool requireMappedData;
  // The only limitaion to having more than 32 dependency layers is this
  unsigned long resourceLifespan;
};
struct Buffer {
  VkDeviceSize size = 0;
  VkDeviceSize offset = 0;
  VkBuffer buffer;
  VmaAllocation deviceMemory;
  void *mappedData = nullptr;
};

// Vulkan Resources are self contained in VulkanDevice
// VulkanDevice handles all resource creation/deletion
class VulkanDevice {
 private:
  class ResourceReservation {
   private:
    // Divide Into Blocks
    // Once memory is reserved it stays reserved, because we know exactly the
    // lifespan we don't reserve where it is not used
    struct MemoryBlock {
      uint32_t size;
      uint32_t offset;
      MemoryBlock(uint32_t size, uint32_t offset)
          : size(size), offset(offset) {}
    };

    struct MemoryReservation {
      std::vector<MemoryBlock> freeStorage;
      std::vector<MemoryBlock> usedStorage;
      MemoryReservation(uint32_t size) {
        freeStorage = std::vector<MemoryBlock>(1, MemoryBlock(size, 0));
      }
    };

   public:
    VkMemoryRequirements localMemReq;
    std::vector<MemoryReservation> memory;
    std::vector<std::pair<uint32_t, uint32_t>> resourceIndices;

    ResourceReservation(VkMemoryRequirements, unsigned long range,
                        uint32_t imgIndex);
    bool tryReserve(VkMemoryRequirements, unsigned long range,
                    uint32_t imgIndex);
  };

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

  Image *createImage(const ImageInfo &imageCreateInfo,
                     const VkMemoryPropertyFlagBits &memoryFlags,
                     bool renderResource);
  std::vector<Image *> createAliasedImages(
      std::vector<ImageInfo> imageCreateInfos);
  Buffer *createBuffer(const BufferInfo &bufferCreateInfo,
                       const VkMemoryPropertyFlagBits &memoryFlags,
                       bool renderResource);
  std::vector<Buffer *> createAliasedBuffers(
      std::vector<BufferInfo> bufferCreateInfos);
};
}  // namespace core_internal::rendering::vulkan