#include "ShadowPass.h"

#include <array>
vks::ShadowPass::ShadowPass(VulkanDevice* device, VkFormat depthFormat,
                            uint32_t imageCount, float shadowMapSize,
                            std::string vertexShaderPath)
    : device(device) {
  this->colorFormat = colorFormat;
  this->depthFormat = depthFormat;
  this->vertexShaderPath = vertexShaderPath;

  std::array<VkAttachmentDescription, 1> attchmentDescriptions = {};
  // Depth attachment
  attchmentDescriptions[0].format = VK_FORMAT_D16_UNORM;
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
                                     nullptr, &renderPass));

  VkFilter shadowmap_filter =
      vks::tools::formatIsFilterable(
          device->physicalDevice, VK_FORMAT_D16_UNORM, VK_IMAGE_TILING_OPTIMAL)
          ? VK_FILTER_LINEAR
          : VK_FILTER_NEAREST;

  // Create sampler to sample from the color attachments
  VkSamplerCreateInfo samplerInfo = vks::initializers::samplerCreateInfo();
  samplerInfo.magFilter = shadowmap_filter;
  samplerInfo.minFilter = shadowmap_filter;
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

  // Depth attachment
  VkImageCreateInfo image = vks::initializers::imageCreateInfo();
  image.imageType = VK_IMAGE_TYPE_2D;
  image.format = VK_FORMAT_D16_UNORM;
  image.extent.width = shadowMapSize;
  image.extent.height = shadowMapSize;
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
  depthStencilView.format = VK_FORMAT_D16_UNORM;
  depthStencilView.flags = 0;
  depthStencilView.subresourceRange = {};
  depthStencilView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
  depthStencilView.subresourceRange.baseMipLevel = 0;
  depthStencilView.subresourceRange.levelCount = 1;
  depthStencilView.subresourceRange.baseArrayLayer = 0;
  depthStencilView.subresourceRange.layerCount = 1;

  framebuffers.resize(imageCount);
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
    VkImageView attachments[1];
    attachments[0] = framebuffers[i].depth.view;

    VkFramebufferCreateInfo fbufCreateInfo =
        vks::initializers::framebufferCreateInfo();
    fbufCreateInfo.renderPass = renderPass;
    fbufCreateInfo.attachmentCount = 1;
    fbufCreateInfo.pAttachments = attachments;
    fbufCreateInfo.width = shadowMapSize;
    fbufCreateInfo.height = shadowMapSize;
    fbufCreateInfo.layers = 1;

    VK_CHECK_RESULT(vkCreateFramebuffer(device->logicalDevice, &fbufCreateInfo,
                                        nullptr, &framebuffers[i].framebuffer));

    // Fill a descriptor for later use in a descriptor set
    framebuffers[i].descriptor.imageLayout =
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    framebuffers[i].descriptor.imageView = framebuffers[i].depth.view;
    framebuffers[i].descriptor.sampler = sampler;
  }
}

vks::ShadowPass::~ShadowPass() {
  for (size_t i = 0; i < framebuffers.size(); i++) {
    vkDestroyImage(device->logicalDevice, framebuffers[i].depth.image, nullptr);
    vkDestroyImageView(device->logicalDevice, framebuffers[i].depth.view,
                       nullptr);
    vkFreeMemory(device->logicalDevice, framebuffers[i].depth.memory, nullptr);
    vkDestroyFramebuffer(device->logicalDevice, framebuffers[i].framebuffer,
                         nullptr);
  }
  vkDestroyRenderPass(device->logicalDevice, renderPass, nullptr);
  vkDestroySampler(device->logicalDevice, sampler, nullptr);
  vkDestroyPipeline(device->logicalDevice, pipeline, nullptr);
}