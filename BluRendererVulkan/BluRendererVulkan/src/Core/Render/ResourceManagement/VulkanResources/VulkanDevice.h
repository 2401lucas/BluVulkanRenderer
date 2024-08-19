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

#include "../ExternalResources/Debug.hpp"
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
  VkMemoryPropertyFlags memoryFlags;
  // The only limitaion to having more than 64 dependency layers is this
  unsigned long resourceLifespan;
  bool requireSampler = false;
  bool requireImageView = false;
  bool requireMappedData = false;
  VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  SamplerInfo samplerInfo;
  ImageViewInfo imageViewInfo;
  VkDeviceSize offset = 0;
};

class Image {
 public:
  VkImage image = VK_NULL_HANDLE;
  VkImageLayout imageLayout;
  VmaAllocation deviceMemory = VK_NULL_HANDLE;
  VkDeviceSize offset = 0;
  VkImageView view = VK_NULL_HANDLE;
  VkSampler sampler = VK_NULL_HANDLE;
  VkDescriptorImageInfo descriptor;
  VkImageSubresourceRange subresourceRange;
  void *mappedData = nullptr;

  void transitionImageLayout(VkCommandBuffer cmdbuffer,
                             VkImageLayout newImageLayout) {
    transitionImageLayout(cmdbuffer, imageLayout, newImageLayout);
  }

  void transitionImageLayout(VkCommandBuffer cmdbuffer,
                             VkImageLayout oldImageLayout,
                             VkImageLayout newImageLayout) {
    if (oldImageLayout == newImageLayout) return;

    VkAccessFlags2 srcAccessMask = getAccessFlags(imageLayout);
    VkAccessFlags2 dstAccessMask = getAccessFlags(newImageLayout);
    VkPipelineStageFlags2 srcStageMask = getPipelineStageFlags(imageLayout);
    VkPipelineStageFlags2 dstStageMask = getPipelineStageFlags(newImageLayout);

    VkImageMemoryBarrier2 imageMemoryBarrier{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask = srcStageMask,
        .srcAccessMask = srcAccessMask,
        .dstStageMask = dstStageMask,
        .dstAccessMask = dstAccessMask,
        .oldLayout = oldImageLayout,
        .newLayout = newImageLayout,
        .image = image,
        .subresourceRange = subresourceRange,
    };

    VkDependencyFlags dependencyFlags = 0;
    // If both stages are in framebuffer space
    if ((srcStageMask & dstStageMask &
         (VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
          VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
          VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT |
          VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT)) != 0)
      dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    VkDependencyInfo info{
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .dependencyFlags = dependencyFlags,
        .memoryBarrierCount = 0,
        .pMemoryBarriers = nullptr,
        .bufferMemoryBarrierCount = 0,
        .pBufferMemoryBarriers = nullptr,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &imageMemoryBarrier,
    };

    vkCmdPipelineBarrier2(cmdbuffer, &info);

    imageLayout = newImageLayout;
  }

  VkPipelineStageFlags getPipelineStageFlags(VkImageLayout layout) {
    switch (layout) {
      case VK_IMAGE_LAYOUT_UNDEFINED:
        return VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
      case VK_IMAGE_LAYOUT_PREINITIALIZED:
        return VK_PIPELINE_STAGE_HOST_BIT;
      case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
      case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        return VK_PIPELINE_STAGE_TRANSFER_BIT;
      case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        return VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
      case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL:
        return VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
               VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
      case VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR:
        return VK_PIPELINE_STAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR;
      case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        return VK_PIPELINE_STAGE_VERTEX_SHADER_BIT |
               VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
      case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
        return VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
      case VK_IMAGE_LAYOUT_GENERAL:
      default:
        DEBUG_ERROR(
            "Don't know how to get a meaningful VkPipelineStageFlags for "
            "VK_IMAGE_LAYOUT_GENERAL! Don't use it!");
    }
  }

  VkAccessFlags getAccessFlags(VkImageLayout layout) {
    switch (layout) {
      case VK_IMAGE_LAYOUT_UNDEFINED:
      case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
        return 0;
      case VK_IMAGE_LAYOUT_PREINITIALIZED:
        return VK_ACCESS_HOST_WRITE_BIT;
      case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        return VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
               VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
      case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL:
        return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
               VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
      case VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR:
        return VK_ACCESS_FRAGMENT_SHADING_RATE_ATTACHMENT_READ_BIT_KHR;
      case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        return VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
      case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        return VK_ACCESS_TRANSFER_READ_BIT;
      case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        return VK_ACCESS_TRANSFER_WRITE_BIT;
      case VK_IMAGE_LAYOUT_GENERAL:
      default:
        DEBUG_ERROR(
            "Don't know how to get a meaningful VkAccessFlags for "
            "VK_IMAGE_LAYOUT_GENERAL! Don't use it!");
    }
  }
};

struct BufferInfo {
  VkDeviceSize size;
  VkBufferUsageFlags usage;
  VkMemoryPropertyFlags memoryFlags;
  bool requireMappedData;
  // The only limitaion to having more than 32 dependency layers is this
  unsigned long resourceLifespan;
};
struct Buffer {
  VkDeviceSize size = 0;
  VkDeviceSize offset = 0;
  VkBuffer buffer;
  VmaAllocation deviceMemory;
  VkDescriptorBufferInfo descriptor;
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

  Image *createImage(const ImageInfo &imageCreateInfo, bool renderResource);
  std::vector<Image *> createAliasedImages(
      std::vector<ImageInfo> imageCreateInfos);
  Buffer *createBuffer(const BufferInfo &bufferCreateInfo, bool renderResource);
  std::vector<Buffer *> createAliasedBuffers(
      std::vector<BufferInfo> bufferCreateInfos);
};
}  // namespace core_internal::rendering::vulkan