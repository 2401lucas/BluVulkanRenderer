#include "PostProcessingPass.h"

#include <array>

namespace vks {
PostProcessingPass::PostProcessingPass(
    VulkanDevice* device, VkFormat colorFormat, VkFormat depthFormat,
    uint32_t imageCount, float width, float height,
    std::string vertexShaderPath, std::string fragmentShaderPath)
    : device(device) {
  this->colorFormat = colorFormat;
  this->depthFormat = depthFormat;
  this->vertexShaderPath = vertexShaderPath;
  this->fragmentShaderPath = fragmentShaderPath;

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
                                     nullptr, &renderPass));

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
  VK_CHECK_RESULT(
      vkCreateSampler(device->logicalDevice, &samplerInfo, nullptr, &sampler));

  createResources(imageCount, width, height);
}

PostProcessingPass::~PostProcessingPass() {
  cleanResources();
  vkDestroyRenderPass(device->logicalDevice, renderPass, nullptr);
  vkDestroySampler(device->logicalDevice, sampler, nullptr);
  vkDestroyPipeline(device->logicalDevice, pipeline, nullptr);
}

void PostProcessingPass::onResize(uint32_t imageCount, float width,
                                  float height) {
  cleanResources();
  createResources(imageCount, width, height);
}

void PostProcessingPass::cleanResources() {
  for (size_t i = 0; i < framebuffers.size(); i++) {
    vkDestroyImage(device->logicalDevice, framebuffers[i].color.image, nullptr);
    vkDestroyImageView(device->logicalDevice, framebuffers[i].color.view,
                       nullptr);
    vkFreeMemory(device->logicalDevice, framebuffers[i].color.memory, nullptr);

    vkDestroyImage(device->logicalDevice, framebuffers[i].depth.image, nullptr);
    vkDestroyImageView(device->logicalDevice, framebuffers[i].depth.view,
                       nullptr);
    vkFreeMemory(device->logicalDevice, framebuffers[i].depth.memory, nullptr);
    vkDestroyFramebuffer(device->logicalDevice, framebuffers[i].framebuffer,
                         nullptr);
  }
}

void PostProcessingPass::createResources(uint32_t imageCount, float width,
                                         float height) {
  std::array<VkImageView, 2> offscreenAttachments;
  // Color attachment
  VkImageCreateInfo image = vks::initializers::imageCreateInfo();
  image.imageType = VK_IMAGE_TYPE_2D;
  image.format = colorFormat;
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
  colorImageView.format = colorFormat;
  colorImageView.flags = 0;
  colorImageView.subresourceRange = {};
  colorImageView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  colorImageView.subresourceRange.baseMipLevel = 0;
  colorImageView.subresourceRange.levelCount = 1;
  colorImageView.subresourceRange.baseArrayLayer = 0;
  colorImageView.subresourceRange.layerCount = 1;

  framebuffers.resize(imageCount);
  for (uint32_t i = 0; i < imageCount; i++) {
    VK_CHECK_RESULT(vkCreateImage(device->logicalDevice, &image, nullptr,
                                  &framebuffers[i].color.image));
    vkGetImageMemoryRequirements(device->logicalDevice,
                                 framebuffers[i].color.image, &memReqs);
    memAlloc.allocationSize = memReqs.size;
    memAlloc.memoryTypeIndex = device->getMemoryType(
        memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    VK_CHECK_RESULT(vkAllocateMemory(device->logicalDevice, &memAlloc, nullptr,
                                     &framebuffers[i].color.memory));
    VK_CHECK_RESULT(vkBindImageMemory(device->logicalDevice,
                                      framebuffers[i].color.image,
                                      framebuffers[i].color.memory, 0));

    colorImageView.image = framebuffers[i].color.image;
    VK_CHECK_RESULT(vkCreateImageView(device->logicalDevice, &colorImageView,
                                      nullptr, &framebuffers[i].color.view));
  }

  // Depth stencil attachment
  image.format = depthFormat;
  image.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

  VkImageViewCreateInfo depthStencilView =
      vks::initializers::imageViewCreateInfo();
  depthStencilView.viewType = VK_IMAGE_VIEW_TYPE_2D;
  depthStencilView.format = depthFormat;
  depthStencilView.flags = 0;
  depthStencilView.subresourceRange = {};
  depthStencilView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
  if (vks::tools::formatHasStencil(depthFormat)) {
    depthStencilView.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
  }
  depthStencilView.subresourceRange.baseMipLevel = 0;
  depthStencilView.subresourceRange.levelCount = 1;
  depthStencilView.subresourceRange.baseArrayLayer = 0;
  depthStencilView.subresourceRange.layerCount = 1;

  for (uint32_t i = 0; i < imageCount; i++) {
    VK_CHECK_RESULT(vkCreateImage(device->logicalDevice, &image, nullptr,
                                  &framebuffers[i].depth.image));
    vkGetImageMemoryRequirements(device->logicalDevice,
                                 framebuffers[i].depth.image, &memReqs);
    memAlloc.allocationSize = memReqs.size;
    memAlloc.memoryTypeIndex = device->getMemoryType(
        memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    VK_CHECK_RESULT(vkAllocateMemory(device->logicalDevice, &memAlloc, nullptr,
                                     &framebuffers[i].depth.memory));
    VK_CHECK_RESULT(vkBindImageMemory(device->logicalDevice,
                                      framebuffers[i].depth.image,
                                      framebuffers[i].depth.memory, 0));

    depthStencilView.image = framebuffers[i].depth.image;
    VK_CHECK_RESULT(vkCreateImageView(device->logicalDevice, &depthStencilView,
                                      nullptr, &framebuffers[i].depth.view));
  }

  for (uint32_t i = 0; i < imageCount; i++) {
    VkImageView attachments[2];
    attachments[0] = framebuffers[i].color.view;
    attachments[1] = framebuffers[i].depth.view;

    VkFramebufferCreateInfo fbufCreateInfo =
        vks::initializers::framebufferCreateInfo();
    fbufCreateInfo.renderPass = renderPass;
    fbufCreateInfo.attachmentCount = 2;
    fbufCreateInfo.pAttachments = attachments;
    fbufCreateInfo.width = width;
    fbufCreateInfo.height = height;
    fbufCreateInfo.layers = 1;

    VK_CHECK_RESULT(vkCreateFramebuffer(device->logicalDevice, &fbufCreateInfo,
                                        nullptr, &framebuffers[i].framebuffer));

    // Fill a descriptor for later use in a descriptor set
    framebuffers[i].descriptor.imageLayout =
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    framebuffers[i].descriptor.imageView = framebuffers[i].color.view;
    framebuffers[i].descriptor.sampler = sampler;
  }
}

}  // namespace vks