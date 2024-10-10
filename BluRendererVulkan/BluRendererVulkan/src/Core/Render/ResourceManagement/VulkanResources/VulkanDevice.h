#pragma once

#define VK_USE_PLATFORM_WIN32_KHR

#include <assert.h>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

#include <algorithm>
#include <exception>

#include "../../../Tools/Debug.hpp"
#include "VulkanTools.h"

namespace core_internal::rendering {
class Image {
 public:
  // Required
  VkImage image = VK_NULL_HANDLE;
  VkImageView view = VK_NULL_HANDLE;
  VkSampler sampler = VK_NULL_HANDLE;
  VkImageLayout imageLayout;
  VmaAllocation alloc = VK_NULL_HANDLE;
  VkDeviceSize size;
  VkDeviceSize offset = 0;
  VkImageSubresourceRange subresourceRange;
  // Optional
  VkMemoryRequirements memReqs;
  void *mappedData = nullptr;
};

struct Buffer {
  // Required
  VkBuffer buffer;
  VmaAllocation alloc;
  VkDeviceSize size;
  VkDeviceSize offset = 0;
  // Optional
  void *mappedData = nullptr;
  VkDeviceAddress deviceAddress;
};

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

  uint32_t apiVersion = VK_API_VERSION_1_2;
  std::vector<std::string> supportedInstanceExtensions;
  std::vector<std::string> supportedExtensions;

  VkInstance instance;
  VkPhysicalDevice physicalDevice;
  VkDevice logicalDevice;
  VmaAllocator allocator;
  // Command pool used for GPU operations
  VkCommandPool commandPool;
  VkPhysicalDeviceProperties physicalDeviceProperties;
  VkPhysicalDeviceFeatures physicalDeviceFeatures;
  VkPhysicalDeviceFeatures physicalDeviceEnabledFeatures;
  VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;
  std::vector<VkQueueFamilyProperties> queueFamilyProperties;
  std::vector<VkShaderModule> shaderModules;

  VkFormat colorFormat = VK_FORMAT_MAX_ENUM;

  uint32_t getMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties,
                         VkBool32 *memTypeFound = nullptr) const;
  uint32_t getQueueFamilyIndex(VkQueueFlags queueFlags) const;
  VkPhysicalDevice choosePhysicalDevice(std::vector<VkPhysicalDevice> devices);
  int rateDeviceSuitability(VkPhysicalDevice device);
  bool extensionSupported(std::string extension);

 public:
  struct {
    uint32_t graphics;
    uint32_t compute;
    uint32_t transfer;
  } queueFamilyIndices;

  struct {
    VkQueue graphics{VK_NULL_HANDLE};
    VkQueue compute{VK_NULL_HANDLE};
    VkQueue transfer{VK_NULL_HANDLE};
  } queues;

  operator VkInstance() const { return instance; };
  operator VkDevice() const { return logicalDevice; };
  operator VkPhysicalDevice() const { return physicalDevice; };
  operator VmaAllocator() const { return allocator; };
  operator VkPhysicalDeviceProperties() const {
    return physicalDeviceProperties;
  };
  operator VkPhysicalDeviceMemoryProperties() const {
    return physicalDeviceMemoryProperties;
  };

  explicit VulkanDevice(const char *name, bool useValidation,
                        std::vector<const char *> enabledDeviceExtensions,
                        std::vector<const char *> enabledInstanceExtensions,
                        void *pNextChain = nullptr);
  ~VulkanDevice();

  // Helper---------------------------------------------------------------------------------------

  void setPreferredColorFormat(VkFormat);

  VkFormat getSupportedDepthFormat(bool checkSamplingSupport);
  VkFormat getSupportedDepthStencilFormat(bool checkSamplingSupport);

  void waitIdle();

  VkCommandBuffer createCommandBuffer(VkCommandBufferLevel, VkCommandPool,
                                      bool begin);
  VkCommandBuffer createCommandBuffer(VkCommandBufferLevel, bool begin);

  VkImageAspectFlags getImageAspectMask(const VkImageUsageFlags &,
                                        const VkFormat &);
  VkFormat getImageFormat(const VkImageUsageFlags &);
  // Vulkan Resource Management
  // ---------------------------------------------------------------------------------------
  VkPipelineShaderStageCreateInfo loadShader(std::string fileName,
                                             VkShaderStageFlagBits stage);
  void createBuffer(Buffer *buf, const VkBufferCreateInfo &bufCI,
                    VkMemoryPropertyFlags propertyFlags,
                    VmaAllocationCreateFlags vmaFlags, bool mapped = false,
                    bool renderResource = false);
  void createBuffer(Buffer *buf, const VkBufferCreateInfo &bufCI);

  void createImage(Image *img, const VkImageCreateInfo &imgCI,
                   VkMemoryPropertyFlags propertyFlags,
                   VmaAllocationCreateFlags vmaFlags,
                   bool renderResource = false);
  void createImage(Image *img, const VkImageCreateInfo &);
  void createImageView(Image *img, const VkImageViewCreateInfo &);
  void createImageSampler(Image *img, const VkSamplerCreateInfo &);

  void allocateMemory(VmaAllocation *, VmaAllocationCreateInfo &,
                      VkMemoryRequirements &);
  void bindMemory(Image *, VmaAllocation);
  void bindMemory(Buffer *, VmaAllocation);

  void copyAllocToMemory(Buffer *buf, void *dst);
  void copyMemoryToAlloc(Buffer *buf, void *src, VkDeviceSize size);
};
}  // namespace core_internal::rendering