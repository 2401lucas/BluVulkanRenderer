#include "VulkanRenderHelper.h"

#include <array>

namespace vks {
namespace rendering {
vks::VulkanRenderTarget* createDepthRenderTarget(
    vks::VulkanDevice* device, VkFormat depthFormat, VkFilter samplerFilter,
    uint32_t imageCount, float depthMapWidth, float depthMapHeight,
    std::string vertexShaderPath) {
  vks::VulkanRenderTarget* newTarget = new vks::VulkanRenderTarget;
  newTarget->device = device;
  newTarget->vertexShaderPath = vertexShaderPath;
  newTarget->depthFormat = depthFormat;

  std::array<VkAttachmentDescription, 1> attchmentDescriptions = {};
  // Depth attachment
  attchmentDescriptions[0].format = depthFormat;
  attchmentDescriptions[0].samples = VK_SAMPLE_COUNT_1_BIT;
  attchmentDescriptions[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  attchmentDescriptions[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  attchmentDescriptions[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  attchmentDescriptions[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  attchmentDescriptions[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  attchmentDescriptions[0].finalLayout =
      VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

  VkAttachmentReference depthReference = {
      0, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

  VkSubpassDescription subpassDescription = {};
  subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpassDescription.colorAttachmentCount = 0;
  subpassDescription.pDepthStencilAttachment = &depthReference;

  // Use subpass dependencies for layout transitions
  std::array<VkSubpassDependency, 2> dependencies;

  dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
  dependencies[0].dstSubpass = 0;
  dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
  dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
  dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

  dependencies[1].srcSubpass = 0;
  dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
  dependencies[1].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
  dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  dependencies[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
  dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
  dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

  // Create the actual renderpass
  VkRenderPassCreateInfo renderPassInfo = {};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount =
      static_cast<uint32_t>(attchmentDescriptions.size());
  renderPassInfo.pAttachments = attchmentDescriptions.data();
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpassDescription;
  renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
  renderPassInfo.pDependencies = dependencies.data();

  VK_CHECK_RESULT(vkCreateRenderPass(device->logicalDevice, &renderPassInfo,
                                     nullptr, &newTarget->renderPass));

  VkSamplerCreateInfo samplerInfo = vks::initializers::samplerCreateInfo();
  samplerInfo.magFilter = samplerFilter;
  samplerInfo.minFilter = samplerFilter;
  samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  samplerInfo.addressModeV = samplerInfo.addressModeU;
  samplerInfo.addressModeW = samplerInfo.addressModeU;
  samplerInfo.mipLodBias = 0.0f;
  samplerInfo.maxAnisotropy = 1.0f;
  samplerInfo.minLod = 0.0f;
  samplerInfo.maxLod = 1.0f;
  samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
  VK_CHECK_RESULT(vkCreateSampler(device->logicalDevice, &samplerInfo, nullptr,
                                  &newTarget->sampler));

  // Depth attachment
  VkImageCreateInfo image = vks::initializers::imageCreateInfo();
  image.imageType = VK_IMAGE_TYPE_2D;
  image.format = depthFormat;
  image.extent.width = depthMapWidth;
  image.extent.height = depthMapHeight;
  image.extent.depth = 1;
  image.mipLevels = 1;
  image.arrayLayers = 1;
  image.samples = VK_SAMPLE_COUNT_1_BIT;
  image.tiling = VK_IMAGE_TILING_OPTIMAL;
  // We will sample directly from the Depth attachment
  image.usage =
      VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

  VkMemoryAllocateInfo memAlloc = vks::initializers::memoryAllocateInfo();
  VkMemoryRequirements memReqs;

  VkImageViewCreateInfo depthStencilView =
      vks::initializers::imageViewCreateInfo();
  depthStencilView.viewType = VK_IMAGE_VIEW_TYPE_2D;
  depthStencilView.format = depthFormat;
  depthStencilView.flags = 0;
  depthStencilView.subresourceRange = {};
  depthStencilView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
  depthStencilView.subresourceRange.baseMipLevel = 0;
  depthStencilView.subresourceRange.levelCount = 1;
  depthStencilView.subresourceRange.baseArrayLayer = 0;
  depthStencilView.subresourceRange.layerCount = 1;

  newTarget->framebuffers.resize(imageCount);
  for (uint32_t i = 0; i < imageCount; i++) {
    VK_CHECK_RESULT(vkCreateImage(device->logicalDevice, &image, nullptr,
                                  &newTarget->framebuffers[i].depth.image));
    vkGetImageMemoryRequirements(device->logicalDevice,
                                 newTarget->framebuffers[i].depth.image,
                                 &memReqs);
    memAlloc.allocationSize = memReqs.size;
    memAlloc.memoryTypeIndex = device->getMemoryType(
        memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    VK_CHECK_RESULT(vkAllocateMemory(device->logicalDevice, &memAlloc, nullptr,
                                     &newTarget->framebuffers[i].depth.memory));
    VK_CHECK_RESULT(vkBindImageMemory(
        device->logicalDevice, newTarget->framebuffers[i].depth.image,
        newTarget->framebuffers[i].depth.memory, 0));

    depthStencilView.image = newTarget->framebuffers[i].depth.image;
    VK_CHECK_RESULT(vkCreateImageView(device->logicalDevice, &depthStencilView,
                                      nullptr,
                                      &newTarget->framebuffers[i].depth.view));
  }

  for (uint32_t i = 0; i < imageCount; i++) {
    VkImageView attachments[1];
    attachments[0] = newTarget->framebuffers[i].depth.view;

    VkFramebufferCreateInfo fbufCreateInfo =
        vks::initializers::framebufferCreateInfo();
    fbufCreateInfo.renderPass = newTarget->renderPass;
    fbufCreateInfo.attachmentCount = 1;
    fbufCreateInfo.pAttachments = attachments;
    fbufCreateInfo.width = depthMapWidth;
    fbufCreateInfo.height = depthMapHeight;
    fbufCreateInfo.layers = 1;

    VK_CHECK_RESULT(
        vkCreateFramebuffer(device->logicalDevice, &fbufCreateInfo, nullptr,
                            &newTarget->framebuffers[i].framebuffer));

    // Fill a descriptor for later use in a descriptor set
    newTarget->framebuffers[i].descriptor.imageLayout =
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    newTarget->framebuffers[i].descriptor.imageView =
        newTarget->framebuffers[i].depth.view;
    newTarget->framebuffers[i].descriptor.sampler = newTarget->sampler;
  }

  return newTarget;
}

VulkanRenderTarget* createColorDepthRenderTarget(
    vks::VulkanDevice* device, VkFormat colorFormat, VkFormat depthFormat,
    uint32_t imageCount, float width, float height,
    std::string vertexShaderPath, std::string fragmentShaderPath) {
  VulkanRenderTarget* newTarget = new VulkanRenderTarget();
  newTarget->device = device;
  newTarget->vertexShaderPath = vertexShaderPath;
  newTarget->fragmentShaderPath = fragmentShaderPath;
  newTarget->colorFormat = colorFormat;
  newTarget->depthFormat = depthFormat;

  std::array<VkAttachmentDescription, 2> attchmentDescriptions = {};
  // Color attachment
  attchmentDescriptions[0].format = colorFormat;
  attchmentDescriptions[0].samples = VK_SAMPLE_COUNT_1_BIT;
  attchmentDescriptions[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  attchmentDescriptions[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  attchmentDescriptions[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  attchmentDescriptions[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  attchmentDescriptions[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  attchmentDescriptions[0].finalLayout =
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  // Depth attachment
  attchmentDescriptions[1].format = depthFormat;
  attchmentDescriptions[1].samples = VK_SAMPLE_COUNT_1_BIT;
  attchmentDescriptions[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  attchmentDescriptions[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  attchmentDescriptions[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  attchmentDescriptions[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  attchmentDescriptions[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  attchmentDescriptions[1].finalLayout =
      VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkAttachmentReference colorReference = {
      0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
  VkAttachmentReference depthReference = {
      1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

  VkSubpassDescription subpassDescription = {};
  subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpassDescription.colorAttachmentCount = 1;
  subpassDescription.pColorAttachments = &colorReference;
  subpassDescription.pDepthStencilAttachment = &depthReference;

  // Use subpass dependencies for layout transitions
  std::array<VkSubpassDependency, 2> dependencies;

  dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
  dependencies[0].dstSubpass = 0;
  dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
  dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

  dependencies[1].srcSubpass = 0;
  dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
  dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
  dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

  // Create the actual renderpass
  VkRenderPassCreateInfo renderPassInfo = {};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount =
      static_cast<uint32_t>(attchmentDescriptions.size());
  renderPassInfo.pAttachments = attchmentDescriptions.data();
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpassDescription;
  renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
  renderPassInfo.pDependencies = dependencies.data();

  VK_CHECK_RESULT(vkCreateRenderPass(device->logicalDevice, &renderPassInfo,
                                     nullptr, &newTarget->renderPass));

  // Create sampler to sample from the color attachments
  VkSamplerCreateInfo samplerInfo = vks::initializers::samplerCreateInfo();
  samplerInfo.magFilter = VK_FILTER_LINEAR;
  samplerInfo.minFilter = VK_FILTER_LINEAR;
  samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  samplerInfo.addressModeV = samplerInfo.addressModeU;
  samplerInfo.addressModeW = samplerInfo.addressModeU;
  samplerInfo.mipLodBias = 0.0f;
  samplerInfo.maxAnisotropy = 1.0f;
  samplerInfo.minLod = 0.0f;
  samplerInfo.maxLod = 1.0f;
  samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
  VK_CHECK_RESULT(vkCreateSampler(device->logicalDevice, &samplerInfo, nullptr,
                                  &newTarget->sampler));

  recreateColorDepthRenderTargetResources(newTarget, imageCount, width, height);
  return newTarget;
}

void recreateColorDepthRenderTargetResources(vks::VulkanRenderTarget* newTarget,
                                             uint32_t imageCount,
                                             uint32_t width, uint32_t height) {
  for (size_t i = 0; i < newTarget->framebuffers.size(); i++) {
    vkDestroyImage(newTarget->device->logicalDevice,
                   newTarget->framebuffers[i].color.image, nullptr);
    vkDestroyImageView(newTarget->device->logicalDevice,
                       newTarget->framebuffers[i].color.view, nullptr);
    vkFreeMemory(newTarget->device->logicalDevice,
                 newTarget->framebuffers[i].color.memory, nullptr);

    vkDestroyImage(newTarget->device->logicalDevice,
                   newTarget->framebuffers[i].depth.image, nullptr);
    vkDestroyImageView(newTarget->device->logicalDevice,
                       newTarget->framebuffers[i].depth.view, nullptr);
    vkFreeMemory(newTarget->device->logicalDevice,
                 newTarget->framebuffers[i].depth.memory, nullptr);
    vkDestroyFramebuffer(newTarget->device->logicalDevice,
                         newTarget->framebuffers[i].framebuffer, nullptr);
  }

  std::array<VkImageView, 2> offscreenAttachments;
  // Color attachment
  VkImageCreateInfo image = vks::initializers::imageCreateInfo();
  image.imageType = VK_IMAGE_TYPE_2D;
  image.format = newTarget->colorFormat;
  image.extent.width = width;
  image.extent.height = height;
  image.extent.depth = 1;
  image.mipLevels = 1;
  image.arrayLayers = 1;
  image.samples = VK_SAMPLE_COUNT_1_BIT;
  image.tiling = VK_IMAGE_TILING_OPTIMAL;
  // We will sample directly from the color attachment
  image.usage =
      VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

  VkMemoryAllocateInfo memAlloc = vks::initializers::memoryAllocateInfo();
  VkMemoryRequirements memReqs;

  VkImageViewCreateInfo colorImageView =
      vks::initializers::imageViewCreateInfo();
  colorImageView.viewType = VK_IMAGE_VIEW_TYPE_2D;
  colorImageView.format = newTarget->colorFormat;
  colorImageView.flags = 0;
  colorImageView.subresourceRange = {};
  colorImageView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  colorImageView.subresourceRange.baseMipLevel = 0;
  colorImageView.subresourceRange.levelCount = 1;
  colorImageView.subresourceRange.baseArrayLayer = 0;
  colorImageView.subresourceRange.layerCount = 1;

  newTarget->framebuffers.resize(imageCount);
  for (uint32_t i = 0; i < imageCount; i++) {
    VK_CHECK_RESULT(vkCreateImage(newTarget->device->logicalDevice, &image,
                                  nullptr,
                                  &newTarget->framebuffers[i].color.image));
    vkGetImageMemoryRequirements(newTarget->device->logicalDevice,
                                 newTarget->framebuffers[i].color.image,
                                 &memReqs);
    memAlloc.allocationSize = memReqs.size;
    memAlloc.memoryTypeIndex = newTarget->device->getMemoryType(
        memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    VK_CHECK_RESULT(vkAllocateMemory(newTarget->device->logicalDevice,
                                     &memAlloc, nullptr,
                                     &newTarget->framebuffers[i].color.memory));
    VK_CHECK_RESULT(vkBindImageMemory(newTarget->device->logicalDevice,
                                      newTarget->framebuffers[i].color.image,
                                      newTarget->framebuffers[i].color.memory,
                                      0));

    colorImageView.image = newTarget->framebuffers[i].color.image;
    VK_CHECK_RESULT(vkCreateImageView(newTarget->device->logicalDevice,
                                      &colorImageView, nullptr,
                                      &newTarget->framebuffers[i].color.view));
  }

  // Depth stencil attachment
  image.format = newTarget->depthFormat;
  image.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

  VkImageViewCreateInfo depthStencilView =
      vks::initializers::imageViewCreateInfo();
  depthStencilView.viewType = VK_IMAGE_VIEW_TYPE_2D;
  depthStencilView.format = newTarget->depthFormat;
  depthStencilView.flags = 0;
  depthStencilView.subresourceRange = {};
  depthStencilView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
  if (vks::tools::formatHasStencil(newTarget->depthFormat)) {
    depthStencilView.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
  }
  depthStencilView.subresourceRange.baseMipLevel = 0;
  depthStencilView.subresourceRange.levelCount = 1;
  depthStencilView.subresourceRange.baseArrayLayer = 0;
  depthStencilView.subresourceRange.layerCount = 1;

  for (uint32_t i = 0; i < imageCount; i++) {
    VK_CHECK_RESULT(vkCreateImage(newTarget->device->logicalDevice, &image,
                                  nullptr,
                                  &newTarget->framebuffers[i].depth.image));
    vkGetImageMemoryRequirements(newTarget->device->logicalDevice,
                                 newTarget->framebuffers[i].depth.image,
                                 &memReqs);
    memAlloc.allocationSize = memReqs.size;
    memAlloc.memoryTypeIndex = newTarget->device->getMemoryType(
        memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    VK_CHECK_RESULT(vkAllocateMemory(newTarget->device->logicalDevice,
                                     &memAlloc, nullptr,
                                     &newTarget->framebuffers[i].depth.memory));
    VK_CHECK_RESULT(vkBindImageMemory(newTarget->device->logicalDevice,
                                      newTarget->framebuffers[i].depth.image,
                                      newTarget->framebuffers[i].depth.memory,
                                      0));

    depthStencilView.image = newTarget->framebuffers[i].depth.image;
    VK_CHECK_RESULT(vkCreateImageView(newTarget->device->logicalDevice,
                                      &depthStencilView, nullptr,
                                      &newTarget->framebuffers[i].depth.view));
  }

  for (uint32_t i = 0; i < imageCount; i++) {
    VkImageView attachments[2];
    attachments[0] = newTarget->framebuffers[i].color.view;
    attachments[1] = newTarget->framebuffers[i].depth.view;

    VkFramebufferCreateInfo fbufCreateInfo =
        vks::initializers::framebufferCreateInfo();
    fbufCreateInfo.renderPass = newTarget->renderPass;
    fbufCreateInfo.attachmentCount = 2;
    fbufCreateInfo.pAttachments = attachments;
    fbufCreateInfo.width = width;
    fbufCreateInfo.height = height;
    fbufCreateInfo.layers = 1;

    VK_CHECK_RESULT(
        vkCreateFramebuffer(newTarget->device->logicalDevice, &fbufCreateInfo,
                            nullptr, &newTarget->framebuffers[i].framebuffer));

    // Fill a descriptor for later use in a descriptor set
    newTarget->framebuffers[i].descriptor.imageLayout =
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    newTarget->framebuffers[i].descriptor.imageView =
        newTarget->framebuffers[i].color.view;
    newTarget->framebuffers[i].descriptor.sampler = newTarget->sampler;
  }
}

vks::VulkanRenderTarget* createRenderTarget() { return nullptr; }

void createImageFromBuffer(vks::VulkanRenderTarget* target, void** imageData,
                           float dataSize, uint32_t width, uint32_t height,
                           VkFormat imageFormat, VkQueue copyQueue) {
  vks::VulkanRenderTarget::ImageAttachment newAttachment{};
  VkMemoryAllocateInfo memAllocInfo{};
  memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  VkMemoryRequirements memReqs;

  // Use a separate command buffer for texture loading
  VkCommandBuffer copyCmd = target->device->createCommandBuffer(
      VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

  // Create a host-visible staging buffer that contains the raw image data
  VkBuffer stagingBuffer;
  VkDeviceMemory stagingMemory;

  VkBufferCreateInfo bufferCreateInfo{};
  bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferCreateInfo.size = dataSize;
  // This buffer is used as a transfer source for the buffer copy
  bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VK_CHECK_RESULT(vkCreateBuffer(target->device->logicalDevice,
                                 &bufferCreateInfo, nullptr, &stagingBuffer));

  // Get memory requirements for the staging buffer (alignment, memory type
  // bits)
  vkGetBufferMemoryRequirements(target->device->logicalDevice, stagingBuffer,
                                &memReqs);

  memAllocInfo.allocationSize = memReqs.size;
  // Get memory type index for a host visible buffer
  memAllocInfo.memoryTypeIndex = target->device->getMemoryType(
      memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                  VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  VK_CHECK_RESULT(vkAllocateMemory(target->device->logicalDevice, &memAllocInfo,
                                   nullptr, &stagingMemory));
  VK_CHECK_RESULT(vkBindBufferMemory(target->device->logicalDevice,
                                     stagingBuffer, stagingMemory, 0));

  // Copy texture data into staging buffer
  uint8_t* data;
  VK_CHECK_RESULT(vkMapMemory(target->device->logicalDevice, stagingMemory, 0,
                              memReqs.size, 0, (void**)&data));
  memcpy(data, imageData, dataSize);
  vkUnmapMemory(target->device->logicalDevice, stagingMemory);

  VkBufferImageCopy bufferCopyRegion = {};
  bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  bufferCopyRegion.imageSubresource.mipLevel = 0;
  bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
  bufferCopyRegion.imageSubresource.layerCount = 1;
  bufferCopyRegion.imageExtent.width = width;
  bufferCopyRegion.imageExtent.height = height;
  bufferCopyRegion.imageExtent.depth = 1;
  bufferCopyRegion.bufferOffset = 0;

  VkImageCreateInfo imageCreateInfo = vks::initializers::imageCreateInfo();
  imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
  imageCreateInfo.format = imageFormat;
  imageCreateInfo.extent.width = width;
  imageCreateInfo.extent.height = height;
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

  VK_CHECK_RESULT(vkCreateImage(target->device->logicalDevice, &imageCreateInfo,
                                nullptr, &newAttachment.image));

  vkGetImageMemoryRequirements(target->device->logicalDevice,
                               newAttachment.image, &memReqs);

  memAllocInfo.allocationSize = memReqs.size;

  memAllocInfo.memoryTypeIndex = target->device->getMemoryType(
      memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  VK_CHECK_RESULT(vkAllocateMemory(target->device->logicalDevice, &memAllocInfo,
                                   nullptr, &newAttachment.memory));
  VK_CHECK_RESULT(vkBindImageMemory(target->device->logicalDevice,
                                    newAttachment.image, newAttachment.memory,
                                    0));

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
    imageMemoryBarrier.image = newAttachment.image;
    imageMemoryBarrier.subresourceRange = subresourceRange;
    vkCmdPipelineBarrier(copyCmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                         VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0,
                         nullptr, 1, &imageMemoryBarrier);
  }

  vkCmdCopyBufferToImage(copyCmd, stagingBuffer, newAttachment.image,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                         &bufferCopyRegion);

  {
    VkImageMemoryBarrier imageMemoryBarrier{};
    imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    imageMemoryBarrier.image = newAttachment.image;
    imageMemoryBarrier.subresourceRange = subresourceRange;
    vkCmdPipelineBarrier(copyCmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                         VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0,
                         nullptr, 1, &imageMemoryBarrier);
  }

  target->device->flushCommandBuffer(copyCmd, copyQueue);

  // Clean up staging resources
  vkFreeMemory(target->device->logicalDevice, stagingMemory, nullptr);
  vkDestroyBuffer(target->device->logicalDevice, stagingBuffer, nullptr);

  VkImageViewCreateInfo viewCreateInfo{};
  viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  viewCreateInfo.format = imageFormat;
  viewCreateInfo.components = {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G,
                               VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A};
  viewCreateInfo.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
  viewCreateInfo.subresourceRange.levelCount = 1;
  viewCreateInfo.image = newAttachment.image;
  VK_CHECK_RESULT(vkCreateImageView(target->device->logicalDevice,
                                    &viewCreateInfo, nullptr,
                                    &newAttachment.view));

  newAttachment.descriptor.imageLayout =
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  newAttachment.descriptor.imageView = newAttachment.view;
  newAttachment.descriptor.sampler = target->sampler;

  target->passImages.push_back(newAttachment);
}
}  // namespace rendering
}  // namespace vks