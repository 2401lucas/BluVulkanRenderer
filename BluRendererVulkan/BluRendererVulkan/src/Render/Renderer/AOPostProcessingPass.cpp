#include "AOPostProcessingPass.h"

#include <array>

#include "../ResourceManagement/ExternalResources/MathTools.h"

namespace vks {
AOPostProcessingPass::AOPostProcessingPass(
    VulkanDevice *device, VkFormat colorFormat, VkFormat depthFormat,
    uint32_t imageCount, float width, float height,
    std::string vertexShaderPath, std::string fragmentShaderPath,
    VkQueue copyQueue)
    : PostProcessingPass::PostProcessingPass(
          device, colorFormat, depthFormat, imageCount, width, height,
          vertexShaderPath, fragmentShaderPath) {
  uint32_t randTexDim = 8;
  uint32_t randTexPixels = randTexDim * randTexDim;
  // Each Pixel requires an RGB in the form of a float
  uint32_t randTexColors = randTexPixels * 3;

  // If z is always 0, maybe just assume that in the shader and send less data?

  float noiseTextureFloats[192];
  float zero = 0.0f;
  float min = -1.0f;
  float max = 1.0f;
  for (int i = 0; i < 64; i++) {
    int index = i * 3;
    noiseTextureFloats[index] = math::random::randomRange(min, max);
    noiseTextureFloats[index + 1] = math::random::randomRange(min, max);
    noiseTextureFloats[index + 2] = 0.0f;
  }

  VkFormat randTexFormat = colorFormat;  // Could be reduced

  VkMemoryAllocateInfo memAllocInfo{};
  memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  VkMemoryRequirements memReqs;

  // Use a separate command buffer for texture loading
  VkCommandBuffer copyCmd =
      device->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

  // Create a host-visible staging buffer that contains the raw image data
  VkBuffer stagingBuffer;
  VkDeviceMemory stagingMemory;

  VkBufferCreateInfo bufferCreateInfo{};
  bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferCreateInfo.size = randTexPixels * sizeof(float) * 3;
  // This buffer is used as a transfer source for the buffer copy
  bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VK_CHECK_RESULT(vkCreateBuffer(device->logicalDevice, &bufferCreateInfo,
                                 nullptr, &stagingBuffer));

  // Get memory requirements for the staging buffer (alignment, memory type
  // bits)
  vkGetBufferMemoryRequirements(device->logicalDevice, stagingBuffer, &memReqs);

  memAllocInfo.allocationSize = memReqs.size;
  // Get memory type index for a host visible buffer
  memAllocInfo.memoryTypeIndex = device->getMemoryType(
      memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                  VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  VK_CHECK_RESULT(vkAllocateMemory(device->logicalDevice, &memAllocInfo,
                                   nullptr, &stagingMemory));
  VK_CHECK_RESULT(vkBindBufferMemory(device->logicalDevice, stagingBuffer,
                                     stagingMemory, 0));

  // Copy texture data into staging buffer
  uint8_t *data;
  VK_CHECK_RESULT(vkMapMemory(device->logicalDevice, stagingMemory, 0,
                              memReqs.size, 0, (void **)&data));
  memcpy(data, noiseTextureFloats, randTexPixels * sizeof(float) * 3);
  vkUnmapMemory(device->logicalDevice, stagingMemory);

  VkBufferImageCopy bufferCopyRegion = {};
  bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  bufferCopyRegion.imageSubresource.mipLevel = 0;
  bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
  bufferCopyRegion.imageSubresource.layerCount = 1;
  bufferCopyRegion.imageExtent.width = randTexDim;
  bufferCopyRegion.imageExtent.height = randTexDim;
  bufferCopyRegion.imageExtent.depth = 1;
  bufferCopyRegion.bufferOffset = 0;

  VkImageCreateInfo imageCreateInfo = vks::initializers::imageCreateInfo();
  imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
  imageCreateInfo.format = randTexFormat;
  imageCreateInfo.extent.width = randTexDim;
  imageCreateInfo.extent.height = randTexDim;
  imageCreateInfo.extent.depth = 1;
  imageCreateInfo.mipLevels = 1;
  imageCreateInfo.arrayLayers = 1;
  imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  imageCreateInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT;

  // Ensure that the TRANSFER_DST bit is set for staging
  if (!(imageCreateInfo.usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT)) {
    imageCreateInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  }
  VK_CHECK_RESULT(vkCreateImage(device->logicalDevice, &imageCreateInfo,
                                nullptr, &randomTex.image));

  vkGetImageMemoryRequirements(device->logicalDevice, randomTex.image,
                               &memReqs);

  memAllocInfo.allocationSize = memReqs.size;

  memAllocInfo.memoryTypeIndex = device->getMemoryType(
      memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  VK_CHECK_RESULT(vkAllocateMemory(device->logicalDevice, &memAllocInfo,
                                   nullptr, &randomTex.memory));
  VK_CHECK_RESULT(vkBindImageMemory(device->logicalDevice, randomTex.image,
                                    randomTex.memory, 0));

  VkImageSubresourceRange subresourceRange = {};
  subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  subresourceRange.baseMipLevel = 0;
  subresourceRange.levelCount = 1;
  subresourceRange.layerCount = 1;

  // Image barrier for optimal image (target)
  // Optimal image will be used as destination for the copy
  {
    VkImageMemoryBarrier imageMemoryBarrier{};
    imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    imageMemoryBarrier.srcAccessMask = 0;
    imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    imageMemoryBarrier.image = randomTex.image;
    imageMemoryBarrier.subresourceRange = subresourceRange;
    vkCmdPipelineBarrier(copyCmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                         VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0,
                         nullptr, 1, &imageMemoryBarrier);
  }

  vkCmdCopyBufferToImage(copyCmd, stagingBuffer, randomTex.image,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                         &bufferCopyRegion);

  {
    VkImageMemoryBarrier imageMemoryBarrier{};
    imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    imageMemoryBarrier.image = randomTex.image;
    imageMemoryBarrier.subresourceRange = subresourceRange;
    vkCmdPipelineBarrier(copyCmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                         VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0,
                         nullptr, 1, &imageMemoryBarrier);
  }

  device->flushCommandBuffer(copyCmd, copyQueue);

  // Clean up staging resources
  vkFreeMemory(device->logicalDevice, stagingMemory, nullptr);
  vkDestroyBuffer(device->logicalDevice, stagingBuffer, nullptr);

  VkImageViewCreateInfo viewCreateInfo{};
  viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  viewCreateInfo.format = randTexFormat;
  viewCreateInfo.components = {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G,
                               VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A};
  viewCreateInfo.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
  viewCreateInfo.subresourceRange.levelCount = 1;
  viewCreateInfo.image = randomTex.image;
  VK_CHECK_RESULT(vkCreateImageView(device->logicalDevice, &viewCreateInfo,
                                    nullptr, &randomTex.view));

  randomTex.descriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  randomTex.descriptor.imageView = randomTex.view;
  randomTex.descriptor.sampler = sampler;

  mSSAOKernel.resize(16 * 3);
  for (int i = 0; i < 16; i++) {
    int index = i * 3;
    mSSAOKernel[index] = math::random::randomRange(min, max);
    mSSAOKernel[index + 1] = math::random::randomRange(min, max);
    mSSAOKernel[index + 2] = math::random::randomRange(zero, max);

    float scale = (float)i / 16.0f;
    float scaleMul = math::linear::lerp(0.1f, 1.0f, scale * scale);

    mSSAOKernel[index] *= scaleMul;
    mSSAOKernel[index + 1] *= scaleMul;
    mSSAOKernel[index + 2] *= scaleMul;
  }

  staticDescriptorSetLayoutBindings = {
      {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT,
       nullptr},
      {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
       VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
  };
}

AOPostProcessingPass::~AOPostProcessingPass() {
  vkDestroyImage(device->logicalDevice, randomTex.image, nullptr);
  vkDestroyImageView(device->logicalDevice, randomTex.view, nullptr);
  vkFreeMemory(device->logicalDevice, randomTex.memory, nullptr);
  staticPassBuffer.destroy();
  vkDestroyPipelineLayout(device->logicalDevice, pipelineLayout, nullptr);
  vkDestroyDescriptorSetLayout(device->logicalDevice,
                               staticDescriptorSetLayout, nullptr);
}

void AOPostProcessingPass::onRender(VkCommandBuffer curBuf,
                                    uint32_t currentFrameIndex) {
  const std::vector<VkDescriptorSet> descriptorsets = {
      screenTextureDescriptorSet[currentFrameIndex], staticDescriptorSet};

  vkCmdBindDescriptorSets(curBuf, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          pipelineLayout, 0,
                          static_cast<uint32_t>(descriptorsets.size()),
                          descriptorsets.data(), 0, NULL);
  vkCmdBindPipeline(curBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
  vkCmdDraw(curBuf, 3, 1, 0, 0);
}

void AOPostProcessingPass::updateDescriptorSets() {
  std::array<VkWriteDescriptorSet, 2> writeDescriptorSets{};

  writeDescriptorSets[0] = vks::initializers::writeDescriptorSet(
      staticDescriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0,
      &staticPassBuffer.descriptor);

  writeDescriptorSets[1] = vks::initializers::writeDescriptorSet(
      staticDescriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
      &randomTex.descriptor);

  vkUpdateDescriptorSets(device->logicalDevice,
                         static_cast<uint32_t>(writeDescriptorSets.size()),
                         writeDescriptorSets.data(), 0, nullptr);
}

void AOPostProcessingPass::createUbo() {
  VK_CHECK_RESULT(device->createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                           VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                       &staticPassBuffer, sizeof(mSSAOKernel)));
  staticPassBuffer.map();
  updateUbo();
}

void AOPostProcessingPass::updateUbo() {
  memcpy(staticPassBuffer.mapped, &mSSAOKernel, sizeof(mSSAOKernel));
}
}  // namespace vks