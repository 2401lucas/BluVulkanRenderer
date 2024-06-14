#pragma once
#define NOMINMAX

#include <corecrt_math_defines.h>

#include "../ResourceManagement/ExternalResources/VulkanTexture.hpp"
#include "../ResourceManagement/ExternalResources/VulkanglTFModel.h"
#include "BaseRenderer.h"
#include "vkImGui.h"

struct UISettings {
  bool visible = true;
  float scale = 1;
  bool cerberus = true;
  bool displaySkybox = true;
  bool displaySponza = true;
  bool useSampleShading = false;
  int msaaSamples;
  std::array<float, 50> frameTimes{};
  float frameTimeMin = 9999.0f, frameTimeMax = 0.0f;
} uiSettings;

class ForwardRenderer : public BaseRenderer {
 public:
  vkImGUI* imGui = nullptr;

  // Asset Resources
  struct PBRTextures {
    vks::Texture2D albedoMap;
    vks::Texture2D normalMap;
    vks::Texture2D aoMap;
    vks::Texture2D metallicMap;
    vks::Texture2D roughnessMap;
  };

  struct Textures {
    vks::TextureCubeMap environmentCube;
    PBRTextures pbrTextures;
    // Generated at runtime
    vks::Texture2D lutBrdf;
    vks::TextureCubeMap irradianceCube;
    vks::TextureCubeMap prefilteredCube;
  } textures;

  struct Models {
    vkglTF::Model skybox;
    vkglTF::Model cerberus;
    vkglTF::Model sponza;
  } models;

  // Render Resources
  struct {
    vks::Buffer object;
    vks::Buffer skybox;
    vks::Buffer params;
    vks::Buffer postProcessing;
  } uniformBuffers;

  // Should (M?)VP be precomputed
  struct UBOMatrices {
    glm::mat4 projection;
    glm::mat4 model;
    glm::mat4 view;
    glm::vec3 camPos;
  } uboMatrices;

  struct LightInfo {
    glm::vec4 pos;    // XYZ for position, W for light type
    glm::vec4 rot;    // XYZ  for rotation
    glm::vec4 color;  // XYZ for RGB, W for Intensity
  };

  // TODO: More Detailed Lights
  struct UBOParams {
    LightInfo light[1];
    float exposure = 4.5f;
    float gamma = 2.2f;
  } uboParams;

  struct PostProcessingParams {
    bool useFXAA = true;
    float fxaaEdgeThresholdMax = 0.031;
    float fxaaEdgeThresholdMin = 0.062;
    float pixelBlend = 0.5;
  } postProcessingParams;

  struct {
    VkPipelineLayout pbr{VK_NULL_HANDLE};
    VkPipelineLayout postProcessing{VK_NULL_HANDLE};
  } pipelineLayouts;

  struct {
    VkPipeline skybox{VK_NULL_HANDLE};
    VkPipeline pbr{VK_NULL_HANDLE};
    VkPipeline pbrWithSS{VK_NULL_HANDLE};
    VkPipeline postProcessing{VK_NULL_HANDLE};
  } pipelines;

  struct {
    VkDescriptorPool pbr;
    VkDescriptorPool postProcessing;
  } descriptorPools;

  struct {
    VkDescriptorSetLayout pbr{VK_NULL_HANDLE};
    VkDescriptorSetLayout postProcessing{VK_NULL_HANDLE};
  } descriptorSetLayouts;

  struct {
    VkDescriptorSet pbr{VK_NULL_HANDLE};
    VkDescriptorSet skybox{VK_NULL_HANDLE};
    VkDescriptorSet postProcessing{VK_NULL_HANDLE};
  } descriptorSets;

  struct MultiSampleTarget {
    struct {
      VkImage image{VK_NULL_HANDLE};
      VkImageView view{VK_NULL_HANDLE};
      VkDeviceMemory memory{VK_NULL_HANDLE};
    } color;
    struct {
      VkImage image{VK_NULL_HANDLE};
      VkImageView view{VK_NULL_HANDLE};
      VkDeviceMemory memory{VK_NULL_HANDLE};
    } depth;
  } multisampleTarget;

  // Framebuffer for offscreen rendering
  struct FrameBufferAttachment {
    VkImage image;
    VkDeviceMemory memory;
    VkImageView view;
  };

  struct FrameBuffer {
    VkFramebuffer framebuffer;
    FrameBufferAttachment color, depth;
    VkDescriptorImageInfo descriptor;
  };

  struct OffscreenPass {
    VkRenderPass renderPass;
    VkSampler sampler;

    std::vector<FrameBuffer> framebuffers;
  } offscreenPass;

  // Multi-threading
  // 1 Command Pool per FiF?
  std::vector<VkCommandBuffer> uiDrawCmdBuffers;
  std::vector<VkCommandBuffer> postProcessingDrawCmdBuffers;
  std::vector<VkCommandBuffer> pbrDrawCmdBuffers;

  VkExtent2D attachmentSize{};

  ForwardRenderer() : BaseRenderer() {
    name = "Forward Renderer with PBR";
    camera.type = Camera::firstperson;
    camera.movementSpeed = 4.0f;
    camera.rotationSpeed = 0.25f;
    camera.setPosition(glm::vec3(0.0f, 0.0f, -4.8f));
    camera.setRotation(glm::vec3(4.5f, -380.0f, 0.0f));
    camera.setPerspective(60.0f, (float)getWidth() / (float)getHeight(), 0.1f,
                          5000.0f);

    enabledInstanceExtensions.push_back(
        VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

    settings.overlay = true;
    settings.validation = true;

    uboParams.light[0].pos = glm::vec4(0, 0, -2, 0);
    uboParams.light[0].rot = glm::vec4(0, 0, 0, 0);
    uboParams.light[0].color = glm::vec4(1, 0, 1, 1);
  }

  ~ForwardRenderer() {
    vkDestroyPipeline(device, pipelines.skybox, nullptr);
    vkDestroyPipeline(device, pipelines.pbr, nullptr);
    vkDestroyPipeline(device, pipelines.pbrWithSS, nullptr);
    vkDestroyPipeline(device, pipelines.postProcessing, nullptr);

    vkDestroyPipelineLayout(device, pipelineLayouts.pbr, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayouts.postProcessing, nullptr);
    vkDestroyDescriptorSetLayout(device, descriptorSetLayouts.pbr, nullptr);
    vkDestroyDescriptorSetLayout(device, descriptorSetLayouts.postProcessing,
                                 nullptr);
    vkDestroyDescriptorPool(device, descriptorPools.pbr, nullptr);
    vkDestroyDescriptorPool(device, descriptorPools.postProcessing, nullptr);

    vkDestroyImage(device, multisampleTarget.color.image, nullptr);
    vkDestroyImageView(device, multisampleTarget.color.view, nullptr);
    vkFreeMemory(device, multisampleTarget.color.memory, nullptr);
    vkDestroyImage(device, multisampleTarget.depth.image, nullptr);
    vkDestroyImageView(device, multisampleTarget.depth.view, nullptr);
    vkFreeMemory(device, multisampleTarget.depth.memory, nullptr);

    for (uint32_t i = 0; i < swapChain.imageCount; i++) {
      vkDestroyImage(device, offscreenPass.framebuffers[i].color.image,
                     nullptr);
      vkDestroyImageView(device, offscreenPass.framebuffers[i].color.view,
                         nullptr);
      vkFreeMemory(device, offscreenPass.framebuffers[i].color.memory, nullptr);
      vkDestroyImage(device, offscreenPass.framebuffers[i].depth.image,
                     nullptr);
      vkDestroyImageView(device, offscreenPass.framebuffers[i].depth.view,
                         nullptr);
      vkFreeMemory(device, offscreenPass.framebuffers[i].depth.memory, nullptr);

      vkDestroyFramebuffer(device, offscreenPass.framebuffers[i].framebuffer,
                           nullptr);
    }

    vkDestroyRenderPass(device, offscreenPass.renderPass, nullptr);
    vkDestroySampler(device, offscreenPass.sampler, nullptr);

    uniformBuffers.object.destroy();
    uniformBuffers.skybox.destroy();
    uniformBuffers.params.destroy();
    uniformBuffers.postProcessing.destroy();

    models.skybox.destroy(device);
    models.cerberus.destroy(device);
    models.sponza.destroy(device);

    textures.environmentCube.destroy();
    textures.irradianceCube.destroy();
    textures.prefilteredCube.destroy();
    textures.lutBrdf.destroy();
    textures.pbrTextures.albedoMap.destroy();
    textures.pbrTextures.normalMap.destroy();
    textures.pbrTextures.aoMap.destroy();
    textures.pbrTextures.metallicMap.destroy();
    textures.pbrTextures.roughnessMap.destroy();

    delete imGui;
  }

  virtual void getEnabledFeatures() override {
    if (deviceFeatures.sampleRateShading) {
      enabledFeatures.sampleRateShading = VK_TRUE;
    }
    if (deviceFeatures.samplerAnisotropy) {
      enabledFeatures.samplerAnisotropy = VK_TRUE;
    }
  }

  void setupDepthStencil() override {
    if (getMSAASampleCount() == VK_SAMPLE_COUNT_1_BIT) {
      BaseRenderer::setupDepthStencil();
    }
  }

  void setupMultisampleTarget(bool usePreviousResources = false) {
    assert((deviceProperties.limits.framebufferColorSampleCounts &
            getMSAASampleCount()) &&
           (deviceProperties.limits.framebufferDepthSampleCounts &
            getMSAASampleCount()));

    // Destroy MSAA target
    if (usePreviousResources) {
      vkDestroyImage(device, multisampleTarget.color.image, nullptr);
      vkDestroyImageView(device, multisampleTarget.color.view, nullptr);
      vkFreeMemory(device, multisampleTarget.color.memory, nullptr);
      vkDestroyImage(device, multisampleTarget.depth.image, nullptr);
      vkDestroyImageView(device, multisampleTarget.depth.view, nullptr);
      vkFreeMemory(device, multisampleTarget.depth.memory, nullptr);
    }

    // Color Target
    VkImageCreateInfo info = vks::initializers::imageCreateInfo();
    info.imageType = VK_IMAGE_TYPE_2D;
    info.format = swapChain.colorFormat;
    info.extent.width = getWidth();
    info.extent.height = getHeight();
    info.extent.depth = 1;
    info.mipLevels = 1;
    info.arrayLayers = 1;
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    info.tiling = VK_IMAGE_TILING_OPTIMAL;
    info.samples = getMSAASampleCount();
    info.usage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT |
                 VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VK_CHECK_RESULT(
        vkCreateImage(device, &info, nullptr, &multisampleTarget.color.image));

    VkMemoryRequirements memReqs;
    vkGetImageMemoryRequirements(device, multisampleTarget.color.image,
                                 &memReqs);
    VkMemoryAllocateInfo memAlloc = vks::initializers::memoryAllocateInfo();
    memAlloc.allocationSize = memReqs.size;
    // We prefer a lazily allocated memory type as these images do not need to
    // be persited into main memory therefore does not require physical backing
    // storage
    VkBool32 lazyMemTypePresent;
    memAlloc.memoryTypeIndex = vulkanDevice->getMemoryType(
        memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT,
        &lazyMemTypePresent);
    if (!lazyMemTypePresent) {
      // If this is not available, fall back to device local memory
      memAlloc.memoryTypeIndex = vulkanDevice->getMemoryType(
          memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    }
    VK_CHECK_RESULT(vkAllocateMemory(device, &memAlloc, nullptr,
                                     &multisampleTarget.color.memory));
    vkBindImageMemory(device, multisampleTarget.color.image,
                      multisampleTarget.color.memory, 0);

    // Create image view for the MSAA target
    VkImageViewCreateInfo viewInfo = vks::initializers::imageViewCreateInfo();
    viewInfo.image = multisampleTarget.color.image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = swapChain.colorFormat;
    viewInfo.components.r = VK_COMPONENT_SWIZZLE_R;
    viewInfo.components.g = VK_COMPONENT_SWIZZLE_G;
    viewInfo.components.b = VK_COMPONENT_SWIZZLE_B;
    viewInfo.components.a = VK_COMPONENT_SWIZZLE_A;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.layerCount = 1;

    VK_CHECK_RESULT(vkCreateImageView(device, &viewInfo, nullptr,
                                      &multisampleTarget.color.view));

    // Depth target
    info.imageType = VK_IMAGE_TYPE_2D;
    info.format = depthFormat;
    info.extent.width = getWidth();
    info.extent.height = getHeight();
    info.extent.depth = 1;
    info.mipLevels = 1;
    info.arrayLayers = 1;
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    info.tiling = VK_IMAGE_TILING_OPTIMAL;
    info.samples = getMSAASampleCount();
    // Image will only be used as a transient target
    info.usage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT |
                 VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VK_CHECK_RESULT(
        vkCreateImage(device, &info, nullptr, &multisampleTarget.depth.image));

    vkGetImageMemoryRequirements(device, multisampleTarget.depth.image,
                                 &memReqs);
    memAlloc = vks::initializers::memoryAllocateInfo();
    memAlloc.allocationSize = memReqs.size;

    memAlloc.memoryTypeIndex = vulkanDevice->getMemoryType(
        memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT,
        &lazyMemTypePresent);
    if (!lazyMemTypePresent) {
      memAlloc.memoryTypeIndex = vulkanDevice->getMemoryType(
          memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    }

    VK_CHECK_RESULT(vkAllocateMemory(device, &memAlloc, nullptr,
                                     &multisampleTarget.depth.memory));
    vkBindImageMemory(device, multisampleTarget.depth.image,
                      multisampleTarget.depth.memory, 0);

    // Create image view for the MSAA target
    viewInfo.image = multisampleTarget.depth.image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = depthFormat;
    viewInfo.components.r = VK_COMPONENT_SWIZZLE_R;
    viewInfo.components.g = VK_COMPONENT_SWIZZLE_G;
    viewInfo.components.b = VK_COMPONENT_SWIZZLE_B;
    viewInfo.components.a = VK_COMPONENT_SWIZZLE_A;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    if (depthFormat >= VK_FORMAT_D16_UNORM_S8_UINT)
      viewInfo.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.layerCount = 1;

    VK_CHECK_RESULT(vkCreateImageView(device, &viewInfo, nullptr,
                                      &multisampleTarget.depth.view));
  }

  void setupRenderPass() override {
    if (getMSAASampleCount() != VK_SAMPLE_COUNT_1_BIT) {
      attachmentSize = {getWidth(), getHeight()};
      std::array<VkAttachmentDescription, 3> attachments = {};

      // Multisampled attachment that we render to
      attachments[0].format = swapChain.colorFormat;
      attachments[0].samples = getMSAASampleCount();
      attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
      attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      attachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

      // This is the frame buffer attachment to where the multisampled image
      // will be resolved to and which will be presented to the swapchain
      attachments[1].format = swapChain.colorFormat;
      attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
      attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
      attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      attachments[1].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

      // Multisampled depth attachment we render to
      attachments[2].format = depthFormat;
      attachments[2].samples = getMSAASampleCount();
      attachments[2].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
      attachments[2].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      attachments[2].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      attachments[2].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      attachments[2].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      attachments[2].finalLayout =
          VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

      VkAttachmentReference colorReference = {};
      colorReference.attachment = 0;
      colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

      VkAttachmentReference depthReference = {};
      depthReference.attachment = 2;
      depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

      // Resolve attachment reference for the color attachment
      VkAttachmentReference resolveReference = {};
      resolveReference.attachment = 1;
      resolveReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

      VkSubpassDescription subpass = {};
      subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
      subpass.colorAttachmentCount = 1;
      subpass.pColorAttachments = &colorReference;
      // Pass our resolve attachments to the sub pass
      subpass.pResolveAttachments = &resolveReference;
      subpass.pDepthStencilAttachment = &depthReference;

      std::array<VkSubpassDependency, 2> dependencies{};

      // Depth attachment
      dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
      dependencies[0].dstSubpass = 0;
      dependencies[0].srcStageMask =
          VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
          VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
      dependencies[0].dstStageMask =
          VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
          VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
      dependencies[0].srcAccessMask =
          VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
      dependencies[0].dstAccessMask =
          VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |
          VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
      dependencies[0].dependencyFlags = 0;
      // Color attachment
      dependencies[1].srcSubpass = VK_SUBPASS_EXTERNAL;
      dependencies[1].dstSubpass = 0;
      dependencies[1].srcStageMask =
          VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
      dependencies[1].dstStageMask =
          VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
      dependencies[1].srcAccessMask = 0;
      dependencies[1].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                                      VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
      dependencies[1].dependencyFlags = 0;

      VkRenderPassCreateInfo renderPassInfo =
          vks::initializers::renderPassCreateInfo();
      renderPassInfo.attachmentCount =
          static_cast<uint32_t>(attachments.size());
      renderPassInfo.pAttachments = attachments.data();
      renderPassInfo.subpassCount = 1;
      renderPassInfo.pSubpasses = &subpass;
      renderPassInfo.dependencyCount = 2;
      renderPassInfo.pDependencies = dependencies.data();
      VK_CHECK_RESULT(
          vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass));
    } else {
      BaseRenderer::setupRenderPass();
    }
  }

  void setupFrameBuffer() override {
    // If the window has been resized, destroy resources
    if (attachmentSize.width != getWidth() ||
        attachmentSize.height != getHeight()) {
      attachmentSize = {getWidth(), getHeight()};
    }

    // If multisampling is to be used
    if (getMSAASampleCount() != VK_SAMPLE_COUNT_1_BIT) {
      setupMultisampleTarget();

      std::array<VkImageView, 3> attachments = {};
      attachments[0] = multisampleTarget.color.view;
      // attachment[1] = swapchain image
      attachments[2] = multisampleTarget.depth.view;

      VkFramebufferCreateInfo frameBufferCreateInfo = {};
      frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
      frameBufferCreateInfo.pNext = NULL;
      frameBufferCreateInfo.renderPass = renderPass;
      frameBufferCreateInfo.attachmentCount =
          static_cast<uint32_t>(attachments.size());
      frameBufferCreateInfo.pAttachments = attachments.data();
      frameBufferCreateInfo.width = getWidth();
      frameBufferCreateInfo.height = getHeight();
      frameBufferCreateInfo.layers = 1;

      // Create frame buffers for every swap chain image
      frameBuffers.resize(swapChain.imageCount);
      for (uint32_t i = 0; i < frameBuffers.size(); i++) {
        attachments[1] = swapChain.buffers[i].view;
        VK_CHECK_RESULT(vkCreateFramebuffer(device, &frameBufferCreateInfo,
                                            nullptr, &frameBuffers[i]));
      }
    }
    // If not setup additional offscreen framebuffer for post processing
    else {
      BaseRenderer::setupFrameBuffer();
      prepareOffscreen();
      offscreenPass.framebuffers.resize(swapChain.imageCount);
      std::array<VkImageView, 2> offscreenAttachments;
      // Color attachment
      VkImageCreateInfo image = vks::initializers::imageCreateInfo();
      image.imageType = VK_IMAGE_TYPE_2D;
      image.format = swapChain.colorFormat;
      image.extent.width = getWidth();
      image.extent.height = getHeight();
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
      colorImageView.format = swapChain.colorFormat;
      colorImageView.flags = 0;
      colorImageView.subresourceRange = {};
      colorImageView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      colorImageView.subresourceRange.baseMipLevel = 0;
      colorImageView.subresourceRange.levelCount = 1;
      colorImageView.subresourceRange.baseArrayLayer = 0;
      colorImageView.subresourceRange.layerCount = 1;

      for (uint32_t i = 0; i < swapChain.imageCount; i++) {
        VK_CHECK_RESULT(
            vkCreateImage(device, &image, nullptr,
                          &offscreenPass.framebuffers[i].color.image));
        vkGetImageMemoryRequirements(
            device, offscreenPass.framebuffers[i].color.image, &memReqs);
        memAlloc.allocationSize = memReqs.size;
        memAlloc.memoryTypeIndex = vulkanDevice->getMemoryType(
            memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        VK_CHECK_RESULT(
            vkAllocateMemory(device, &memAlloc, nullptr,
                             &offscreenPass.framebuffers[i].color.memory));
        VK_CHECK_RESULT(
            vkBindImageMemory(device, offscreenPass.framebuffers[i].color.image,
                              offscreenPass.framebuffers[i].color.memory, 0));

        colorImageView.image = offscreenPass.framebuffers[i].color.image;
        VK_CHECK_RESULT(
            vkCreateImageView(device, &colorImageView, nullptr,
                              &offscreenPass.framebuffers[i].color.view));
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
        depthStencilView.subresourceRange.aspectMask |=
            VK_IMAGE_ASPECT_STENCIL_BIT;
      }
      depthStencilView.subresourceRange.baseMipLevel = 0;
      depthStencilView.subresourceRange.levelCount = 1;
      depthStencilView.subresourceRange.baseArrayLayer = 0;
      depthStencilView.subresourceRange.layerCount = 1;

      for (uint32_t i = 0; i < swapChain.imageCount; i++) {
        VK_CHECK_RESULT(
            vkCreateImage(device, &image, nullptr,
                          &offscreenPass.framebuffers[i].depth.image));
        vkGetImageMemoryRequirements(
            device, offscreenPass.framebuffers[i].depth.image, &memReqs);
        memAlloc.allocationSize = memReqs.size;
        memAlloc.memoryTypeIndex = vulkanDevice->getMemoryType(
            memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        VK_CHECK_RESULT(
            vkAllocateMemory(device, &memAlloc, nullptr,
                             &offscreenPass.framebuffers[i].depth.memory));
        VK_CHECK_RESULT(
            vkBindImageMemory(device, offscreenPass.framebuffers[i].depth.image,
                              offscreenPass.framebuffers[i].depth.memory, 0));

        depthStencilView.image = offscreenPass.framebuffers[i].depth.image;
        VK_CHECK_RESULT(
            vkCreateImageView(device, &depthStencilView, nullptr,
                              &offscreenPass.framebuffers[i].depth.view));
      }

      for (uint32_t i = 0; i < swapChain.imageCount; i++) {
        VkImageView attachments[2];
        attachments[0] = offscreenPass.framebuffers[i].color.view;
        attachments[1] = offscreenPass.framebuffers[i].depth.view;

        VkFramebufferCreateInfo fbufCreateInfo =
            vks::initializers::framebufferCreateInfo();
        fbufCreateInfo.renderPass = offscreenPass.renderPass;
        fbufCreateInfo.attachmentCount = 2;
        fbufCreateInfo.pAttachments = attachments;
        fbufCreateInfo.width = getWidth();
        ;
        fbufCreateInfo.height = getHeight();
        fbufCreateInfo.layers = 1;

        VK_CHECK_RESULT(
            vkCreateFramebuffer(device, &fbufCreateInfo, nullptr,
                                &offscreenPass.framebuffers[i].framebuffer));

        // Fill a descriptor for later use in a descriptor set
        offscreenPass.framebuffers[i].descriptor.imageLayout =
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        offscreenPass.framebuffers[i].descriptor.imageView =
            offscreenPass.framebuffers[i].color.view;
        offscreenPass.framebuffers[i].descriptor.sampler =
            offscreenPass.sampler;
      }
    }
  }

  void prepareOffscreen() {
    // Create a separate render pass for the offscreen rendering as it may
    // differ from the one used for scene rendering
    std::array<VkAttachmentDescription, 2> attchmentDescriptions = {};
    // Color attachment
    attchmentDescriptions[0].format = swapChain.colorFormat;
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
    dependencies[0].dstStageMask =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    dependencies[1].srcSubpass = 0;
    dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
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

    VK_CHECK_RESULT(vkCreateRenderPass(device, &renderPassInfo, nullptr,
                                       &offscreenPass.renderPass));

    // Create sampler to sample from the color attachments
    VkSamplerCreateInfo sampler = vks::initializers::samplerCreateInfo();
    sampler.magFilter = VK_FILTER_LINEAR;
    sampler.minFilter = VK_FILTER_LINEAR;
    sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler.addressModeV = sampler.addressModeU;
    sampler.addressModeW = sampler.addressModeU;
    sampler.mipLodBias = 0.0f;
    sampler.maxAnisotropy = 1.0f;
    sampler.minLod = 0.0f;
    sampler.maxLod = 1.0f;
    sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    VK_CHECK_RESULT(
        vkCreateSampler(device, &sampler, nullptr, &offscreenPass.sampler));
  }

  void createCommandBuffers() override {
    BaseRenderer::createCommandBuffers();
    uiDrawCmdBuffers.resize(swapChain.imageCount);
    postProcessingDrawCmdBuffers.resize(swapChain.imageCount);
    pbrDrawCmdBuffers.resize(swapChain.imageCount);
    computeCmdBuffers.resize(swapChain.imageCount);

    VkCommandBufferAllocateInfo secondaryGraphicsCmdBufAllocateInfo =
        vks::initializers::commandBufferAllocateInfo(
            graphicsCmdPool, VK_COMMAND_BUFFER_LEVEL_SECONDARY,
            static_cast<uint32_t>(uiDrawCmdBuffers.size()));

    VK_CHECK_RESULT(vkAllocateCommandBuffers(
        device, &secondaryGraphicsCmdBufAllocateInfo, uiDrawCmdBuffers.data()));
    VK_CHECK_RESULT(
        vkAllocateCommandBuffers(device, &secondaryGraphicsCmdBufAllocateInfo,
                                 postProcessingDrawCmdBuffers.data()));
    VK_CHECK_RESULT(
        vkAllocateCommandBuffers(device, &secondaryGraphicsCmdBufAllocateInfo,
                                 pbrDrawCmdBuffers.data()));

    VkCommandBufferAllocateInfo secondaryComputeCmdBufAllocateInfo =
        vks::initializers::commandBufferAllocateInfo(
            computeCmdPool, VK_COMMAND_BUFFER_LEVEL_SECONDARY,
            static_cast<uint32_t>(computeCmdBuffers.size()));

    VK_CHECK_RESULT(vkAllocateCommandBuffers(
        device, &secondaryComputeCmdBufAllocateInfo, computeCmdBuffers.data()));
  }

  void destroyCommandBuffers() override {
    BaseRenderer::destroyCommandBuffers();

    vkFreeCommandBuffers(device, graphicsCmdPool,
                         static_cast<uint32_t>(uiDrawCmdBuffers.size()),
                         uiDrawCmdBuffers.data());
    vkFreeCommandBuffers(
        device, graphicsCmdPool,
        static_cast<uint32_t>(postProcessingDrawCmdBuffers.size()),
        postProcessingDrawCmdBuffers.data());
  }

  // Starts a new imGui frame and sets up windows and ui elements
  void newUIFrame(bool updateFrameGraph) {
    ImGui::NewFrame();

    // Debug window
    ImGui::SetWindowPos(ImVec2(20 * uiSettings.scale, 20 * uiSettings.scale),
                        ImGuiCond_FirstUseEver);
    ImGui::SetWindowSize(ImVec2(300 * uiSettings.scale, 300 * uiSettings.scale),
                         ImGuiCond_Always);
    ImGui::TextUnformatted(getTitle());
    ImGui::TextUnformatted(deviceProperties.deviceName);
    ImGui::Text("Vulkan API %i.%i.%i",
                VK_API_VERSION_MAJOR(deviceProperties.apiVersion),
                VK_API_VERSION_MINOR(deviceProperties.apiVersion),
                VK_API_VERSION_PATCH(deviceProperties.apiVersion));
    ImGui::Text("%s %s", imGui->driverProperties.driverName,
                imGui->driverProperties.driverInfo);

    // Update frame time display
    if (updateFrameGraph) {
      std::rotate(uiSettings.frameTimes.begin(),
                  uiSettings.frameTimes.begin() + 1,
                  uiSettings.frameTimes.end());
      float frameTime = 1000.0f / (frameTimer * 1000.0f);
      uiSettings.frameTimes.back() = frameTime;
      if (frameTime < uiSettings.frameTimeMin) {
        uiSettings.frameTimeMin = frameTime;
      }
      if (frameTime > uiSettings.frameTimeMax) {
        uiSettings.frameTimeMax = frameTime;
      }
    }

    ImGui::PlotLines("Frame Times", &uiSettings.frameTimes[0], 50, 0, "",
                     uiSettings.frameTimeMin, uiSettings.frameTimeMax,
                     ImVec2(0, 80));

    ImGui::Text("Camera");
    ImGui::InputFloat3("position", &camera.position.x);
    ImGui::InputFloat3("rotation", &camera.rotation.x);

    // Example settings window
    ImGui::SetNextWindowPos(
        ImVec2(20 * uiSettings.scale, 360 * uiSettings.scale),
        ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(
        ImVec2(300 * uiSettings.scale, 200 * uiSettings.scale),
        ImGuiCond_FirstUseEver);
    ImGui::Begin("Scene Settings");
    ImGui::Checkbox("Display Cerberus", &uiSettings.cerberus);
    ImGui::Checkbox("Display Level", &uiSettings.displaySponza);
    if (ImGui::CollapsingHeader("Rendering Settings")) {
      ImGui::Checkbox("Display Skybox", &uiSettings.displaySkybox);
      // ImGui::Checkbox("Use Sample Shading", &uiSettings.useSampleShading);
      /*if (ImGui::Checkbox("Use FXAA", &postProcessingParams.useFXAA)) {
        updateParams();
      }*/

      if (ImGui::DragFloat("FXAA Blend Amount",
                           &postProcessingParams.pixelBlend, 0.01f, 0, 1,
                           "%.03f")) {
        updateParams();
      }
      if (ImGui::DragFloat("fxaaEdgeThresholdMin",
                           &postProcessingParams.fxaaEdgeThresholdMin, 0.0001f,
                           0.0625, 0.25, "%.0003f")) {
        updateParams();
      }
      if (ImGui::DragFloat("fxaaEdgeThresholdMax",
                           &postProcessingParams.fxaaEdgeThresholdMax, 0.0001f,
                           0.03125, 0.125, "%.0003f")) {
        updateParams();
      }

      if (ImGui::CollapsingHeader("Light Settings")) {
        if (ImGui::InputFloat3("position", &uboParams.light[0].pos.x)) {
          updateParams();
        }
        if (ImGui::InputFloat3("rotation", &uboParams.light[0].rot.x)) {
          updateParams();
        }
        if (ImGui::ColorPicker4("Color", &uboParams.light[0].color.x)) {
          updateParams();
        }
      }
    }

    if (ImGui::Combo("UI style", &imGui->selectedStyle,
                     "Vulkan\0Classic\0Dark\0Light\0")) {
      imGui->setStyle(imGui->selectedStyle);
    }

    ImGui::End();

    // Render to generate draw buffers
    ImGui::Render();
  }

  void buildPostProcessingCommandBuffer() {
    VkCommandBufferInheritanceInfo inheritanceInfo =
        vks::initializers::commandBufferInheritanceInfo();
    inheritanceInfo.renderPass = renderPass;
    inheritanceInfo.framebuffer = frameBuffers[currentFrameIndex];

    VkCommandBufferBeginInfo cmdBufInfo =
        vks::initializers::commandBufferBeginInfo();
    cmdBufInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
    cmdBufInfo.pInheritanceInfo = &inheritanceInfo;

    VkCommandBuffer currentCommandBuffer =
        postProcessingDrawCmdBuffers[currentFrameIndex];
    vkResetCommandBuffer(currentCommandBuffer, 0);
    VK_CHECK_RESULT(vkBeginCommandBuffer(currentCommandBuffer, &cmdBufInfo));

    VkViewport viewport = vks::initializers::viewport(
        (float)getWidth(), (float)getHeight(), 0.0f, 1.0f);
    vkCmdSetViewport(currentCommandBuffer, 0, 1, &viewport);
    VkRect2D scissor = vks::initializers::rect2D(getWidth(), getHeight(), 0, 0);
    vkCmdSetScissor(currentCommandBuffer, 0, 1, &scissor);

    VkDeviceSize offsets[1] = {0};

    vkCmdBindDescriptorSets(currentCommandBuffer,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipelineLayouts.postProcessing, 0, 1,
                            &descriptorSets.postProcessing, 0, NULL);
    vkCmdBindPipeline(currentCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                      pipelines.postProcessing);

    // Draw triangle
    vkCmdDraw(currentCommandBuffer, 3, 1, 0, 0);

    VK_CHECK_RESULT(vkEndCommandBuffer(currentCommandBuffer));
  }

  void buildUICommandBuffer() {
    VkCommandBufferInheritanceInfo inheritanceInfo =
        vks::initializers::commandBufferInheritanceInfo();
    inheritanceInfo.renderPass = renderPass;
    inheritanceInfo.framebuffer = frameBuffers[currentImageIndex];

    VkCommandBufferBeginInfo cmdBufInfo =
        vks::initializers::commandBufferBeginInfo();
    cmdBufInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
    cmdBufInfo.pInheritanceInfo = &inheritanceInfo;

    VkCommandBuffer currentCommandBuffer = uiDrawCmdBuffers[currentFrameIndex];

    vkResetCommandBuffer(currentCommandBuffer, 0);
    VK_CHECK_RESULT(vkBeginCommandBuffer(currentCommandBuffer, &cmdBufInfo));

    imGui->drawFrame(currentCommandBuffer, currentFrameIndex);

    VK_CHECK_RESULT(vkEndCommandBuffer(currentCommandBuffer));
  }

  void buildPbrCommandBuffer() {
    VkCommandBufferInheritanceInfo inheritanceInfo =
        vks::initializers::commandBufferInheritanceInfo();
    inheritanceInfo.renderPass = offscreenPass.renderPass;
    inheritanceInfo.framebuffer =
        offscreenPass.framebuffers[currentImageIndex].framebuffer;

    VkCommandBufferBeginInfo cmdBufInfo =
        vks::initializers::commandBufferBeginInfo();
    cmdBufInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
    cmdBufInfo.pInheritanceInfo = &inheritanceInfo;

    VkCommandBuffer currentCommandBuffer = pbrDrawCmdBuffers[currentFrameIndex];

    vkResetCommandBuffer(currentCommandBuffer, 0);
    VK_CHECK_RESULT(vkBeginCommandBuffer(currentCommandBuffer, &cmdBufInfo));

    VkViewport viewport = vks::initializers::viewport(
        (float)getWidth(), (float)getHeight(), 0.0f, 1.0f);
    vkCmdSetViewport(currentCommandBuffer, 0, 1, &viewport);
    VkRect2D scissor = vks::initializers::rect2D(getWidth(), getHeight(), 0, 0);
    vkCmdSetScissor(currentCommandBuffer, 0, 1, &scissor);

    VkDeviceSize offsets[1] = {0};

    // Skybox
    if (uiSettings.displaySkybox) {
      vkCmdBindDescriptorSets(
          currentCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
          pipelineLayouts.pbr, 0, 1, &descriptorSets.skybox, 0, NULL);
      vkCmdBindPipeline(currentCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                        pipelines.skybox);
      models.skybox.draw(currentCommandBuffer);
    }

    // PBR Objects
    if (uiSettings.cerberus) {
      vkCmdBindDescriptorSets(
          currentCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
          pipelineLayouts.pbr, 0, 1, &descriptorSets.pbr, 0, NULL);
      vkCmdBindPipeline(currentCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                        (vulkanDevice->features.sampleRateShading &&
                         uiSettings.useSampleShading)
                            ? pipelines.pbrWithSS
                            : pipelines.pbr);
      models.cerberus.draw(currentCommandBuffer);
    }

    VK_CHECK_RESULT(vkEndCommandBuffer(currentCommandBuffer));
  }

  void buildCommandBuffer() override {
    // Contains the list of secondary command buffers to be submitted
    std::vector<VkCommandBuffer> secondaryCmdBufs;

    BaseRenderer::buildCommandBuffer();
    VkCommandBufferBeginInfo cmdBufInfo =
        vks::initializers::commandBufferBeginInfo();

    VkClearValue clearValues[3];
    clearValues[0].color = {{1.0f, 1.0f, 1.0f, 1.0f}};
    clearValues[1].color = {{1.0f, 1.0f, 1.0f, 1.0f}};
    clearValues[2].depthStencil = {1.0f, 0};

    VkRenderPassBeginInfo renderPassBeginInfo =
        vks::initializers::renderPassBeginInfo();

    renderPassBeginInfo.renderPass = offscreenPass.renderPass;
    renderPassBeginInfo.renderArea.offset.x = 0;
    renderPassBeginInfo.renderArea.offset.y = 0;
    renderPassBeginInfo.renderArea.extent.width = getWidth();
    renderPassBeginInfo.renderArea.extent.height = getHeight();
    renderPassBeginInfo.clearValueCount = 3;
    renderPassBeginInfo.pClearValues = clearValues;

    newUIFrame((frameCounter == 0));
    imGui->updateBuffers(currentFrameIndex);

    renderPassBeginInfo.framebuffer =
        offscreenPass.framebuffers[currentFrameIndex].framebuffer;

    VkCommandBuffer currentCommandBuffer = drawCmdBuffers[currentFrameIndex];

    VK_CHECK_RESULT(vkBeginCommandBuffer(currentCommandBuffer, &cmdBufInfo));

    vkCmdBeginRenderPass(currentCommandBuffer, &renderPassBeginInfo,
                         VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

    buildPbrCommandBuffer();

    secondaryCmdBufs.push_back(pbrDrawCmdBuffers[currentFrameIndex]);

    // Execute render commands from the secondary command buffer
    vkCmdExecuteCommands(currentCommandBuffer,
                         static_cast<uint32_t>(secondaryCmdBufs.size()),
                         secondaryCmdBufs.data());
    secondaryCmdBufs.clear();
    vkCmdEndRenderPass(currentCommandBuffer);

    renderPassBeginInfo.renderPass = renderPass;
    renderPassBeginInfo.framebuffer = frameBuffers[currentFrameIndex];

    vkCmdBeginRenderPass(currentCommandBuffer, &renderPassBeginInfo,
                         VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

    buildPostProcessingCommandBuffer();
    buildUICommandBuffer();

    secondaryCmdBufs.push_back(postProcessingDrawCmdBuffers[currentFrameIndex]);
    secondaryCmdBufs.push_back(uiDrawCmdBuffers[currentFrameIndex]);

    // Execute render commands from the secondary command buffer
    vkCmdExecuteCommands(currentCommandBuffer,
                         static_cast<uint32_t>(secondaryCmdBufs.size()),
                         secondaryCmdBufs.data());

    vkCmdEndRenderPass(currentCommandBuffer);

    VK_CHECK_RESULT(vkEndCommandBuffer(currentCommandBuffer));
  }

  void setupDescriptors() {
    // Descriptor Pool
    std::vector<VkDescriptorPoolSize> poolSizes = {
        vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                              4),
        vks::initializers::descriptorPoolSize(
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 16)};
    VkDescriptorPoolCreateInfo descriptorPoolInfo =
        vks::initializers::descriptorPoolCreateInfo(poolSizes, 2);
    VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr,
                                           &descriptorPools.pbr));

    // Descriptor set layout
    std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
        vks::initializers::descriptorSetLayoutBinding(
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0),
        vks::initializers::descriptorSetLayoutBinding(
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 1),
        vks::initializers::descriptorSetLayoutBinding(
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            VK_SHADER_STAGE_FRAGMENT_BIT, 2),
        vks::initializers::descriptorSetLayoutBinding(
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            VK_SHADER_STAGE_FRAGMENT_BIT, 3),
        vks::initializers::descriptorSetLayoutBinding(
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            VK_SHADER_STAGE_FRAGMENT_BIT, 4),
        vks::initializers::descriptorSetLayoutBinding(
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            VK_SHADER_STAGE_FRAGMENT_BIT, 5),
        vks::initializers::descriptorSetLayoutBinding(
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            VK_SHADER_STAGE_FRAGMENT_BIT, 6),
        vks::initializers::descriptorSetLayoutBinding(
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            VK_SHADER_STAGE_FRAGMENT_BIT, 7),
        vks::initializers::descriptorSetLayoutBinding(
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            VK_SHADER_STAGE_FRAGMENT_BIT, 8),
        vks::initializers::descriptorSetLayoutBinding(
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            VK_SHADER_STAGE_FRAGMENT_BIT, 9),
    };

    VkDescriptorSetLayoutCreateInfo descriptorLayout =
        vks::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings);
    VK_CHECK_RESULT(vkCreateDescriptorSetLayout(
        device, &descriptorLayout, nullptr, &descriptorSetLayouts.pbr));

    // Descriptor sets
    VkDescriptorSetAllocateInfo allocInfo =
        vks::initializers::descriptorSetAllocateInfo(
            descriptorPools.pbr, &descriptorSetLayouts.pbr, 1);

    // Objects
    VK_CHECK_RESULT(
        vkAllocateDescriptorSets(device, &allocInfo, &descriptorSets.pbr));
    std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
        vks::initializers::writeDescriptorSet(
            descriptorSets.pbr, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0,
            &uniformBuffers.object.descriptor),
        vks::initializers::writeDescriptorSet(
            descriptorSets.pbr, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1,
            &uniformBuffers.params.descriptor),
        vks::initializers::writeDescriptorSet(
            descriptorSets.pbr, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2,
            &textures.irradianceCube.descriptor),
        vks::initializers::writeDescriptorSet(
            descriptorSets.pbr, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3,
            &textures.lutBrdf.descriptor),
        vks::initializers::writeDescriptorSet(
            descriptorSets.pbr, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4,
            &textures.prefilteredCube.descriptor),
        vks::initializers::writeDescriptorSet(
            descriptorSets.pbr, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 5,
            &textures.pbrTextures.albedoMap.descriptor),
        vks::initializers::writeDescriptorSet(
            descriptorSets.pbr, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 6,
            &textures.pbrTextures.normalMap.descriptor),
        vks::initializers::writeDescriptorSet(
            descriptorSets.pbr, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 7,
            &textures.pbrTextures.aoMap.descriptor),
        vks::initializers::writeDescriptorSet(
            descriptorSets.pbr, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 8,
            &textures.pbrTextures.metallicMap.descriptor),
        vks::initializers::writeDescriptorSet(
            descriptorSets.pbr, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 9,
            &textures.pbrTextures.roughnessMap.descriptor),
    };
    vkUpdateDescriptorSets(device,
                           static_cast<uint32_t>(writeDescriptorSets.size()),
                           writeDescriptorSets.data(), 0, NULL);

    // Sky box
    VK_CHECK_RESULT(
        vkAllocateDescriptorSets(device, &allocInfo, &descriptorSets.skybox));
    writeDescriptorSets = {
        vks::initializers::writeDescriptorSet(
            descriptorSets.skybox, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0,
            &uniformBuffers.skybox.descriptor),
        vks::initializers::writeDescriptorSet(
            descriptorSets.skybox, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1,
            &uniformBuffers.params.descriptor),
        vks::initializers::writeDescriptorSet(
            descriptorSets.skybox, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2,
            &textures.environmentCube.descriptor),
    };
    vkUpdateDescriptorSets(device,
                           static_cast<uint32_t>(writeDescriptorSets.size()),
                           writeDescriptorSets.data(), 0, NULL);
    // Post Processing

    // Descriptor Pool
    std::vector<VkDescriptorPoolSize> postProcessingPoolSizes = {
        vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                              1),
        vks::initializers::descriptorPoolSize(
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3)};
    descriptorPoolInfo =
        vks::initializers::descriptorPoolCreateInfo(postProcessingPoolSizes, 2);
    VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr,
                                           &descriptorPools.postProcessing));

    // Descriptor set layout
    std::vector<VkDescriptorSetLayoutBinding> postProcessingSetLayoutBindings =
        {
            vks::initializers::descriptorSetLayoutBinding(
                VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0),
            vks::initializers::descriptorSetLayoutBinding(
                VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                VK_SHADER_STAGE_FRAGMENT_BIT, 1),
        };

    descriptorLayout = vks::initializers::descriptorSetLayoutCreateInfo(
        postProcessingSetLayoutBindings);
    VK_CHECK_RESULT(
        vkCreateDescriptorSetLayout(device, &descriptorLayout, nullptr,
                                    &descriptorSetLayouts.postProcessing));

    allocInfo = vks::initializers::descriptorSetAllocateInfo(
        descriptorPools.postProcessing, &descriptorSetLayouts.postProcessing,
        1);
    VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo,
                                             &descriptorSets.postProcessing));

    writeDescriptorSets = {
        // Binding 0: Vertex shader uniform buffer
        vks::initializers::writeDescriptorSet(
            descriptorSets.postProcessing, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0,
            &uniformBuffers.postProcessing.descriptor),
    };

    for (uint32_t i = 0; i < 1; i++) {
      // Binding 1: Fragment shader texture sampler
      VkWriteDescriptorSet wSet = vks::initializers::writeDescriptorSet(
          descriptorSets.postProcessing,
          VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
          &offscreenPass.framebuffers[i].descriptor);
      writeDescriptorSets.push_back(wSet);
    }

    vkUpdateDescriptorSets(device,
                           static_cast<uint32_t>(writeDescriptorSets.size()),
                           writeDescriptorSets.data(), 0, NULL);
  }

  void preparePipelines() {
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyState =
        vks::initializers::pipelineInputAssemblyStateCreateInfo(
            VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
    VkPipelineRasterizationStateCreateInfo rasterizationState =
        vks::initializers::pipelineRasterizationStateCreateInfo(
            VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT,
            VK_FRONT_FACE_COUNTER_CLOCKWISE);
    VkPipelineColorBlendAttachmentState blendAttachmentState =
        vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE);
    VkPipelineColorBlendStateCreateInfo colorBlendState =
        vks::initializers::pipelineColorBlendStateCreateInfo(
            1, &blendAttachmentState);
    VkPipelineDepthStencilStateCreateInfo depthStencilState =
        vks::initializers::pipelineDepthStencilStateCreateInfo(
            VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
    VkPipelineViewportStateCreateInfo viewportState =
        vks::initializers::pipelineViewportStateCreateInfo(1, 1);
    VkPipelineMultisampleStateCreateInfo multisampleState =
        vks::initializers::pipelineMultisampleStateCreateInfo(
            getMSAASampleCount());
    std::vector<VkDynamicState> dynamicStateEnables = {
        VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicState =
        vks::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables);

    std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo =
        vks::initializers::pipelineLayoutCreateInfo(&descriptorSetLayouts.pbr,
                                                    1);
    VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo,
                                           nullptr, &pipelineLayouts.pbr));
    VkGraphicsPipelineCreateInfo pipelineCI =
        vks::initializers::graphicsPipelineCreateInfo(pipelineLayouts.pbr,
                                                      offscreenPass.renderPass);

    pipelineCI.pInputAssemblyState = &inputAssemblyState;
    pipelineCI.pRasterizationState = &rasterizationState;
    pipelineCI.pColorBlendState = &colorBlendState;
    pipelineCI.pMultisampleState = &multisampleState;
    pipelineCI.pViewportState = &viewportState;
    pipelineCI.pDepthStencilState = &depthStencilState;
    pipelineCI.pDynamicState = &dynamicState;
    pipelineCI.stageCount = static_cast<uint32_t>(shaderStages.size());
    pipelineCI.pStages = shaderStages.data();
    pipelineCI.pVertexInputState = vkglTF::Vertex::getPipelineVertexInputState(
        {vkglTF::VertexComponent::Position, vkglTF::VertexComponent::Normal,
         vkglTF::VertexComponent::UV, vkglTF::VertexComponent::Tangent});

    // Skybox pipeline (background cube)
    rasterizationState.cullMode = VK_CULL_MODE_FRONT_BIT;
    shaderStages[0] =
        loadShader("shaders/skybox.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
    shaderStages[1] =
        loadShader("shaders/skybox.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
    VK_CHECK_RESULT(vkCreateGraphicsPipelines(
        device, pipelineCache, 1, &pipelineCI, nullptr, &pipelines.skybox));

    rasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;

    // PBR pipeline
    shaderStages[0] =
        loadShader("shaders/pbrtexture.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
    shaderStages[1] =
        loadShader("shaders/pbrtexture.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

    VK_CHECK_RESULT(vkCreateGraphicsPipelines(
        device, pipelineCache, 1, &pipelineCI, nullptr, &pipelines.pbr));

    // MSAA with sample shading pipeline
    // Sample shading enables per-sample shading to avoid shader aliasing and
    // smooth out e.g. high frequency texture maps Note: This will trade
    // performance for a more stable image
    if (vulkanDevice->features.sampleRateShading) {
      // Enable per - sample shading(instead of per - fragment)
      multisampleState.sampleShadingEnable = VK_TRUE;
      // Minimum fraction for sample shading
      multisampleState.minSampleShading = 0.25f;
      VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1,
                                                &pipelineCI, nullptr,
                                                &pipelines.pbrWithSS));
    }

    pipelineLayoutCreateInfo = vks::initializers::pipelineLayoutCreateInfo(
        &descriptorSetLayouts.postProcessing, 1);
    VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo,
                                           nullptr,
                                           &pipelineLayouts.postProcessing));

    VkGraphicsPipelineCreateInfo postProcessingpipelineCI =
        vks::initializers::graphicsPipelineCreateInfo(
            pipelineLayouts.postProcessing, renderPass);

    VkPipelineVertexInputStateCreateInfo emptyInputState{};
    emptyInputState.sType =
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    emptyInputState.vertexAttributeDescriptionCount = 0;
    emptyInputState.pVertexAttributeDescriptions = nullptr;
    emptyInputState.vertexBindingDescriptionCount = 0;
    emptyInputState.pVertexBindingDescriptions = nullptr;

    rasterizationState.cullMode = VK_CULL_MODE_FRONT_BIT;
    postProcessingpipelineCI.pInputAssemblyState = &inputAssemblyState;
    postProcessingpipelineCI.pRasterizationState = &rasterizationState;
    postProcessingpipelineCI.pColorBlendState = &colorBlendState;
    postProcessingpipelineCI.pMultisampleState = &multisampleState;
    postProcessingpipelineCI.pViewportState = &viewportState;
    postProcessingpipelineCI.pDepthStencilState = &depthStencilState;
    postProcessingpipelineCI.pDynamicState = &dynamicState;
    postProcessingpipelineCI.stageCount =
        static_cast<uint32_t>(shaderStages.size());
    postProcessingpipelineCI.pStages = shaderStages.data();
    postProcessingpipelineCI.pVertexInputState = &emptyInputState;

    // Post Processing
    shaderStages[0] =
        loadShader("shaders/fxaa.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
    shaderStages[1] =
        loadShader("shaders/fxaa.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

    VK_CHECK_RESULT(
        vkCreateGraphicsPipelines(device, nullptr, 1, &postProcessingpipelineCI,
                                  nullptr, &pipelines.postProcessing));
  }

  // Generate a BRDF integration map used as a look-up-table (Roughness/NdotV)
  // for Irradiance map (0..1 Scale/Bias)
  void generateBRDFLUT() {
    auto tStart = std::chrono::high_resolution_clock::now();

    const VkFormat format =
        VK_FORMAT_R16G16_SFLOAT;  // R16G16 is supported pretty much everywhere
    const int32_t dim = 512;

    // Image
    VkImageCreateInfo imageCI = vks::initializers::imageCreateInfo();
    imageCI.imageType = VK_IMAGE_TYPE_2D;
    imageCI.format = format;
    imageCI.extent.width = dim;
    imageCI.extent.height = dim;
    imageCI.extent.depth = 1;
    imageCI.mipLevels = 1;
    imageCI.arrayLayers = 1;
    imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCI.usage =
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    VK_CHECK_RESULT(
        vkCreateImage(device, &imageCI, nullptr, &textures.lutBrdf.image));
    VkMemoryAllocateInfo memAlloc = vks::initializers::memoryAllocateInfo();
    VkMemoryRequirements memReqs;
    vkGetImageMemoryRequirements(device, textures.lutBrdf.image, &memReqs);
    memAlloc.allocationSize = memReqs.size;
    memAlloc.memoryTypeIndex = vulkanDevice->getMemoryType(
        memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    VK_CHECK_RESULT(vkAllocateMemory(device, &memAlloc, nullptr,
                                     &textures.lutBrdf.deviceMemory));
    VK_CHECK_RESULT(vkBindImageMemory(device, textures.lutBrdf.image,
                                      textures.lutBrdf.deviceMemory, 0));
    // Image view
    VkImageViewCreateInfo viewCI = vks::initializers::imageViewCreateInfo();
    viewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewCI.format = format;
    viewCI.subresourceRange = {};
    viewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewCI.subresourceRange.levelCount = 1;
    viewCI.subresourceRange.layerCount = 1;
    viewCI.image = textures.lutBrdf.image;
    VK_CHECK_RESULT(
        vkCreateImageView(device, &viewCI, nullptr, &textures.lutBrdf.view));
    // Sampler
    VkSamplerCreateInfo samplerCI = vks::initializers::samplerCreateInfo();
    samplerCI.magFilter = VK_FILTER_LINEAR;
    samplerCI.minFilter = VK_FILTER_LINEAR;
    samplerCI.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerCI.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerCI.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerCI.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerCI.minLod = 0.0f;
    samplerCI.maxLod = 1.0f;
    samplerCI.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    VK_CHECK_RESULT(vkCreateSampler(device, &samplerCI, nullptr,
                                    &textures.lutBrdf.sampler));

    textures.lutBrdf.descriptor.imageView = textures.lutBrdf.view;
    textures.lutBrdf.descriptor.sampler = textures.lutBrdf.sampler;
    textures.lutBrdf.descriptor.imageLayout =
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    textures.lutBrdf.device = vulkanDevice;

    // FB, Att, RP, Pipe, etc.
    VkAttachmentDescription attDesc = {};
    // Color attachment
    attDesc.format = format;
    attDesc.samples = VK_SAMPLE_COUNT_1_BIT;
    attDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attDesc.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    VkAttachmentReference colorReference = {
        0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

    VkSubpassDescription subpassDescription = {};
    subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescription.colorAttachmentCount = 1;
    subpassDescription.pColorAttachments = &colorReference;

    // Use subpass dependencies for layout transitions
    std::array<VkSubpassDependency, 2> dependencies;
    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[0].dstStageMask =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                                    VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    dependencies[1].srcSubpass = 0;
    dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                                    VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    // Create the actual renderpass
    VkRenderPassCreateInfo renderPassCI =
        vks::initializers::renderPassCreateInfo();
    renderPassCI.attachmentCount = 1;
    renderPassCI.pAttachments = &attDesc;
    renderPassCI.subpassCount = 1;
    renderPassCI.pSubpasses = &subpassDescription;
    renderPassCI.dependencyCount = 2;
    renderPassCI.pDependencies = dependencies.data();

    VkRenderPass renderpass;
    VK_CHECK_RESULT(
        vkCreateRenderPass(device, &renderPassCI, nullptr, &renderpass));

    VkFramebufferCreateInfo framebufferCI =
        vks::initializers::framebufferCreateInfo();
    framebufferCI.renderPass = renderpass;
    framebufferCI.attachmentCount = 1;
    framebufferCI.pAttachments = &textures.lutBrdf.view;
    framebufferCI.width = dim;
    framebufferCI.height = dim;
    framebufferCI.layers = 1;

    VkFramebuffer framebuffer;
    VK_CHECK_RESULT(
        vkCreateFramebuffer(device, &framebufferCI, nullptr, &framebuffer));

    // Descriptors
    VkDescriptorSetLayout descriptorsetlayout;
    std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {};
    VkDescriptorSetLayoutCreateInfo descriptorsetlayoutCI =
        vks::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings);
    VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorsetlayoutCI,
                                                nullptr, &descriptorsetlayout));

    // Descriptor Pool
    std::vector<VkDescriptorPoolSize> poolSizes = {
        vks::initializers::descriptorPoolSize(
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1)};
    VkDescriptorPoolCreateInfo descriptorPoolCI =
        vks::initializers::descriptorPoolCreateInfo(poolSizes, 2);
    VkDescriptorPool descriptorpool;
    VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolCI, nullptr,
                                           &descriptorpool));

    // Descriptor sets
    VkDescriptorSet descriptorset;
    VkDescriptorSetAllocateInfo allocInfo =
        vks::initializers::descriptorSetAllocateInfo(descriptorpool,
                                                     &descriptorsetlayout, 1);
    VK_CHECK_RESULT(
        vkAllocateDescriptorSets(device, &allocInfo, &descriptorset));

    // Pipeline layout
    VkPipelineLayout pipelinelayout;
    VkPipelineLayoutCreateInfo pipelineLayoutCI =
        vks::initializers::pipelineLayoutCreateInfo(&descriptorsetlayout, 1);
    VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCI, nullptr,
                                           &pipelinelayout));

    // Pipeline
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyState =
        vks::initializers::pipelineInputAssemblyStateCreateInfo(
            VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
    VkPipelineRasterizationStateCreateInfo rasterizationState =
        vks::initializers::pipelineRasterizationStateCreateInfo(
            VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE,
            VK_FRONT_FACE_COUNTER_CLOCKWISE);
    VkPipelineColorBlendAttachmentState blendAttachmentState =
        vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE);
    VkPipelineColorBlendStateCreateInfo colorBlendState =
        vks::initializers::pipelineColorBlendStateCreateInfo(
            1, &blendAttachmentState);
    VkPipelineDepthStencilStateCreateInfo depthStencilState =
        vks::initializers::pipelineDepthStencilStateCreateInfo(
            VK_FALSE, VK_FALSE, VK_COMPARE_OP_LESS_OR_EQUAL);
    VkPipelineViewportStateCreateInfo viewportState =
        vks::initializers::pipelineViewportStateCreateInfo(1, 1);
    VkPipelineMultisampleStateCreateInfo multisampleState =
        vks::initializers::pipelineMultisampleStateCreateInfo(
            VK_SAMPLE_COUNT_1_BIT);
    std::vector<VkDynamicState> dynamicStateEnables = {
        VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicState =
        vks::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables);
    VkPipelineVertexInputStateCreateInfo emptyInputState =
        vks::initializers::pipelineVertexInputStateCreateInfo();
    std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;

    VkGraphicsPipelineCreateInfo pipelineCI =
        vks::initializers::graphicsPipelineCreateInfo(pipelinelayout,
                                                      renderpass);
    pipelineCI.pInputAssemblyState = &inputAssemblyState;
    pipelineCI.pRasterizationState = &rasterizationState;
    pipelineCI.pColorBlendState = &colorBlendState;
    pipelineCI.pMultisampleState = &multisampleState;
    pipelineCI.pViewportState = &viewportState;
    pipelineCI.pDepthStencilState = &depthStencilState;
    pipelineCI.pDynamicState = &dynamicState;
    pipelineCI.stageCount = 2;
    pipelineCI.pStages = shaderStages.data();
    pipelineCI.pVertexInputState = &emptyInputState;

    // Look-up-table (from BRDF) pipeline
    shaderStages[0] =
        loadShader("shaders/genbrdflut.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
    shaderStages[1] =
        loadShader("shaders/genbrdflut.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
    VkPipeline pipeline;
    VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1,
                                              &pipelineCI, nullptr, &pipeline));

    // Render
    VkClearValue clearValues[1];
    clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};

    VkRenderPassBeginInfo renderPassBeginInfo =
        vks::initializers::renderPassBeginInfo();
    renderPassBeginInfo.renderPass = renderpass;
    renderPassBeginInfo.renderArea.extent.width = dim;
    renderPassBeginInfo.renderArea.extent.height = dim;
    renderPassBeginInfo.clearValueCount = 1;
    renderPassBeginInfo.pClearValues = clearValues;
    renderPassBeginInfo.framebuffer = framebuffer;

    VkCommandBuffer cmdBuf = vulkanDevice->createCommandBuffer(
        VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
    vkCmdBeginRenderPass(cmdBuf, &renderPassBeginInfo,
                         VK_SUBPASS_CONTENTS_INLINE);
    VkViewport viewport =
        vks::initializers::viewport((float)dim, (float)dim, 0.0f, 1.0f);
    VkRect2D scissor = vks::initializers::rect2D(dim, dim, 0, 0);
    vkCmdSetViewport(cmdBuf, 0, 1, &viewport);
    vkCmdSetScissor(cmdBuf, 0, 1, &scissor);
    vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    vkCmdDraw(cmdBuf, 3, 1, 0, 0);
    vkCmdEndRenderPass(cmdBuf);
    vulkanDevice->flushCommandBuffer(cmdBuf, graphicsQueue);

    vkQueueWaitIdle(graphicsQueue);

    vkDestroyPipeline(device, pipeline, nullptr);
    vkDestroyPipelineLayout(device, pipelinelayout, nullptr);
    vkDestroyRenderPass(device, renderpass, nullptr);
    vkDestroyFramebuffer(device, framebuffer, nullptr);
    vkDestroyDescriptorSetLayout(device, descriptorsetlayout, nullptr);
    vkDestroyDescriptorPool(device, descriptorpool, nullptr);

    auto tEnd = std::chrono::high_resolution_clock::now();
    auto tDiff =
        std::chrono::duration<double, std::milli>(tEnd - tStart).count();
    std::cout << "Generating BRDF LUT took " << tDiff << " ms" << std::endl;
  }

  // Generate an irradiance cube map from the environment cube map
  void generateIrradianceCube() {
    auto tStart = std::chrono::high_resolution_clock::now();

    const VkFormat format = VK_FORMAT_R32G32B32A32_SFLOAT;
    const int32_t dim = 64;
    const uint32_t numMips = static_cast<uint32_t>(floor(log2(dim))) + 1;

    // Pre-filtered cube map
    // Image
    VkImageCreateInfo imageCI = vks::initializers::imageCreateInfo();
    imageCI.imageType = VK_IMAGE_TYPE_2D;
    imageCI.format = format;
    imageCI.extent.width = dim;
    imageCI.extent.height = dim;
    imageCI.extent.depth = 1;
    imageCI.mipLevels = numMips;
    imageCI.arrayLayers = 6;
    imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCI.usage =
        VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    imageCI.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    VK_CHECK_RESULT(vkCreateImage(device, &imageCI, nullptr,
                                  &textures.irradianceCube.image));
    VkMemoryAllocateInfo memAlloc = vks::initializers::memoryAllocateInfo();
    VkMemoryRequirements memReqs;
    vkGetImageMemoryRequirements(device, textures.irradianceCube.image,
                                 &memReqs);
    memAlloc.allocationSize = memReqs.size;
    memAlloc.memoryTypeIndex = vulkanDevice->getMemoryType(
        memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    VK_CHECK_RESULT(vkAllocateMemory(device, &memAlloc, nullptr,
                                     &textures.irradianceCube.deviceMemory));
    VK_CHECK_RESULT(vkBindImageMemory(device, textures.irradianceCube.image,
                                      textures.irradianceCube.deviceMemory, 0));
    // Image view
    VkImageViewCreateInfo viewCI = vks::initializers::imageViewCreateInfo();
    viewCI.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
    viewCI.format = format;
    viewCI.subresourceRange = {};
    viewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewCI.subresourceRange.levelCount = numMips;
    viewCI.subresourceRange.layerCount = 6;
    viewCI.image = textures.irradianceCube.image;
    VK_CHECK_RESULT(vkCreateImageView(device, &viewCI, nullptr,
                                      &textures.irradianceCube.view));
    // Sampler
    VkSamplerCreateInfo samplerCI = vks::initializers::samplerCreateInfo();
    samplerCI.magFilter = VK_FILTER_LINEAR;
    samplerCI.minFilter = VK_FILTER_LINEAR;
    samplerCI.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerCI.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerCI.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerCI.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerCI.minLod = 0.0f;
    samplerCI.maxLod = static_cast<float>(numMips);
    samplerCI.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    VK_CHECK_RESULT(vkCreateSampler(device, &samplerCI, nullptr,
                                    &textures.irradianceCube.sampler));

    textures.irradianceCube.descriptor.imageView = textures.irradianceCube.view;
    textures.irradianceCube.descriptor.sampler =
        textures.irradianceCube.sampler;
    textures.irradianceCube.descriptor.imageLayout =
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    textures.irradianceCube.device = vulkanDevice;

    // FB, Att, RP, Pipe, etc.
    VkAttachmentDescription attDesc = {};
    // Color attachment
    attDesc.format = format;
    attDesc.samples = VK_SAMPLE_COUNT_1_BIT;
    attDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attDesc.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    VkAttachmentReference colorReference = {
        0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

    VkSubpassDescription subpassDescription = {};
    subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescription.colorAttachmentCount = 1;
    subpassDescription.pColorAttachments = &colorReference;

    // Use subpass dependencies for layout transitions
    std::array<VkSubpassDependency, 2> dependencies;
    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[0].dstStageMask =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                                    VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    dependencies[1].srcSubpass = 0;
    dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                                    VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    // Renderpass
    VkRenderPassCreateInfo renderPassCI =
        vks::initializers::renderPassCreateInfo();
    renderPassCI.attachmentCount = 1;
    renderPassCI.pAttachments = &attDesc;
    renderPassCI.subpassCount = 1;
    renderPassCI.pSubpasses = &subpassDescription;
    renderPassCI.dependencyCount = 2;
    renderPassCI.pDependencies = dependencies.data();
    VkRenderPass renderpass;
    VK_CHECK_RESULT(
        vkCreateRenderPass(device, &renderPassCI, nullptr, &renderpass));

    struct {
      VkImage image;
      VkImageView view;
      VkDeviceMemory memory;
      VkFramebuffer framebuffer;
    } offscreen;

    // Offfscreen framebuffer
    {
      // Color attachment
      VkImageCreateInfo imageCreateInfo = vks::initializers::imageCreateInfo();
      imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
      imageCreateInfo.format = format;
      imageCreateInfo.extent.width = dim;
      imageCreateInfo.extent.height = dim;
      imageCreateInfo.extent.depth = 1;
      imageCreateInfo.mipLevels = 1;
      imageCreateInfo.arrayLayers = 1;
      imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
      imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
      imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      imageCreateInfo.usage =
          VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
      imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
      VK_CHECK_RESULT(
          vkCreateImage(device, &imageCreateInfo, nullptr, &offscreen.image));

      VkMemoryAllocateInfo memAlloc = vks::initializers::memoryAllocateInfo();
      VkMemoryRequirements memReqs;
      vkGetImageMemoryRequirements(device, offscreen.image, &memReqs);
      memAlloc.allocationSize = memReqs.size;
      memAlloc.memoryTypeIndex = vulkanDevice->getMemoryType(
          memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
      VK_CHECK_RESULT(
          vkAllocateMemory(device, &memAlloc, nullptr, &offscreen.memory));
      VK_CHECK_RESULT(
          vkBindImageMemory(device, offscreen.image, offscreen.memory, 0));

      VkImageViewCreateInfo colorImageView =
          vks::initializers::imageViewCreateInfo();
      colorImageView.viewType = VK_IMAGE_VIEW_TYPE_2D;
      colorImageView.format = format;
      colorImageView.flags = 0;
      colorImageView.subresourceRange = {};
      colorImageView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      colorImageView.subresourceRange.baseMipLevel = 0;
      colorImageView.subresourceRange.levelCount = 1;
      colorImageView.subresourceRange.baseArrayLayer = 0;
      colorImageView.subresourceRange.layerCount = 1;
      colorImageView.image = offscreen.image;
      VK_CHECK_RESULT(
          vkCreateImageView(device, &colorImageView, nullptr, &offscreen.view));

      VkFramebufferCreateInfo fbufCreateInfo =
          vks::initializers::framebufferCreateInfo();
      fbufCreateInfo.renderPass = renderpass;
      fbufCreateInfo.attachmentCount = 1;
      fbufCreateInfo.pAttachments = &offscreen.view;
      fbufCreateInfo.width = dim;
      fbufCreateInfo.height = dim;
      fbufCreateInfo.layers = 1;
      VK_CHECK_RESULT(vkCreateFramebuffer(device, &fbufCreateInfo, nullptr,
                                          &offscreen.framebuffer));

      VkCommandBuffer layoutCmd = vulkanDevice->createCommandBuffer(
          VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
      vks::tools::setImageLayout(
          layoutCmd, offscreen.image, VK_IMAGE_ASPECT_COLOR_BIT,
          VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
      vulkanDevice->flushCommandBuffer(layoutCmd, graphicsQueue, true);
    }

    // Descriptors
    VkDescriptorSetLayout descriptorsetlayout;
    std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
        vks::initializers::descriptorSetLayoutBinding(
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            VK_SHADER_STAGE_FRAGMENT_BIT, 0),
    };
    VkDescriptorSetLayoutCreateInfo descriptorsetlayoutCI =
        vks::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings);
    VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorsetlayoutCI,
                                                nullptr, &descriptorsetlayout));

    // Descriptor Pool
    std::vector<VkDescriptorPoolSize> poolSizes = {
        vks::initializers::descriptorPoolSize(
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1)};
    VkDescriptorPoolCreateInfo descriptorPoolCI =
        vks::initializers::descriptorPoolCreateInfo(poolSizes, 2);
    VkDescriptorPool descriptorpool;
    VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolCI, nullptr,
                                           &descriptorpool));

    // Descriptor sets
    VkDescriptorSet descriptorset;
    VkDescriptorSetAllocateInfo allocInfo =
        vks::initializers::descriptorSetAllocateInfo(descriptorpool,
                                                     &descriptorsetlayout, 1);
    VK_CHECK_RESULT(
        vkAllocateDescriptorSets(device, &allocInfo, &descriptorset));
    VkWriteDescriptorSet writeDescriptorSet =
        vks::initializers::writeDescriptorSet(
            descriptorset, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0,
            &textures.environmentCube.descriptor);
    vkUpdateDescriptorSets(device, 1, &writeDescriptorSet, 0, nullptr);

    // Pipeline layout
    struct PushBlock {
      glm::mat4 mvp;
      // Sampling deltas
      float deltaPhi = (2.0f * float(M_PI)) / 180.0f;
      float deltaTheta = (0.5f * float(M_PI)) / 64.0f;
    } pushBlock;

    VkPipelineLayout pipelinelayout;
    std::vector<VkPushConstantRange> pushConstantRanges = {
        vks::initializers::pushConstantRange(
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            sizeof(PushBlock), 0),
    };
    VkPipelineLayoutCreateInfo pipelineLayoutCI =
        vks::initializers::pipelineLayoutCreateInfo(&descriptorsetlayout, 1);
    pipelineLayoutCI.pushConstantRangeCount = 1;
    pipelineLayoutCI.pPushConstantRanges = pushConstantRanges.data();
    VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCI, nullptr,
                                           &pipelinelayout));

    // Pipeline
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyState =
        vks::initializers::pipelineInputAssemblyStateCreateInfo(
            VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
    VkPipelineRasterizationStateCreateInfo rasterizationState =
        vks::initializers::pipelineRasterizationStateCreateInfo(
            VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE,
            VK_FRONT_FACE_COUNTER_CLOCKWISE);
    VkPipelineColorBlendAttachmentState blendAttachmentState =
        vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE);
    VkPipelineColorBlendStateCreateInfo colorBlendState =
        vks::initializers::pipelineColorBlendStateCreateInfo(
            1, &blendAttachmentState);
    VkPipelineDepthStencilStateCreateInfo depthStencilState =
        vks::initializers::pipelineDepthStencilStateCreateInfo(
            VK_FALSE, VK_FALSE, VK_COMPARE_OP_LESS_OR_EQUAL);
    VkPipelineViewportStateCreateInfo viewportState =
        vks::initializers::pipelineViewportStateCreateInfo(1, 1);
    VkPipelineMultisampleStateCreateInfo multisampleState =
        vks::initializers::pipelineMultisampleStateCreateInfo(
            VK_SAMPLE_COUNT_1_BIT);
    std::vector<VkDynamicState> dynamicStateEnables = {
        VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicState =
        vks::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables);
    std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;

    VkGraphicsPipelineCreateInfo pipelineCI =
        vks::initializers::graphicsPipelineCreateInfo(pipelinelayout,
                                                      renderpass);
    pipelineCI.pInputAssemblyState = &inputAssemblyState;
    pipelineCI.pRasterizationState = &rasterizationState;
    pipelineCI.pColorBlendState = &colorBlendState;
    pipelineCI.pMultisampleState = &multisampleState;
    pipelineCI.pViewportState = &viewportState;
    pipelineCI.pDepthStencilState = &depthStencilState;
    pipelineCI.pDynamicState = &dynamicState;
    pipelineCI.stageCount = 2;
    pipelineCI.pStages = shaderStages.data();
    pipelineCI.renderPass = renderpass;
    pipelineCI.pVertexInputState = vkglTF::Vertex::getPipelineVertexInputState(
        {vkglTF::VertexComponent::Position, vkglTF::VertexComponent::Normal,
         vkglTF::VertexComponent::UV});

    shaderStages[0] =
        loadShader("shaders/filtercube.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
    shaderStages[1] = loadShader("shaders/irradiancecube.frag.spv",
                                 VK_SHADER_STAGE_FRAGMENT_BIT);
    VkPipeline pipeline;
    VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1,
                                              &pipelineCI, nullptr, &pipeline));

    VkClearValue clearValues[1];
    clearValues[0].color = {{0.0f, 0.0f, 0.2f, 0.0f}};

    VkRenderPassBeginInfo renderPassBeginInfo =
        vks::initializers::renderPassBeginInfo();
    // Reuse render pass from example pass
    renderPassBeginInfo.renderPass = renderpass;
    renderPassBeginInfo.framebuffer = offscreen.framebuffer;
    renderPassBeginInfo.renderArea.extent.width = dim;
    renderPassBeginInfo.renderArea.extent.height = dim;
    renderPassBeginInfo.clearValueCount = 1;
    renderPassBeginInfo.pClearValues = clearValues;

    std::vector<glm::mat4> matrices = {
        // POSITIVE_X
        glm::rotate(glm::rotate(glm::mat4(1.0f), glm::radians(90.0f),
                                glm::vec3(0.0f, 1.0f, 0.0f)),
                    glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
        // NEGATIVE_X
        glm::rotate(glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f),
                                glm::vec3(0.0f, 1.0f, 0.0f)),
                    glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
        // POSITIVE_Y
        glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f),
                    glm::vec3(1.0f, 0.0f, 0.0f)),
        // NEGATIVE_Y
        glm::rotate(glm::mat4(1.0f), glm::radians(90.0f),
                    glm::vec3(1.0f, 0.0f, 0.0f)),
        // POSITIVE_Z
        glm::rotate(glm::mat4(1.0f), glm::radians(180.0f),
                    glm::vec3(1.0f, 0.0f, 0.0f)),
        // NEGATIVE_Z
        glm::rotate(glm::mat4(1.0f), glm::radians(180.0f),
                    glm::vec3(0.0f, 0.0f, 1.0f)),
    };

    VkCommandBuffer cmdBuf = vulkanDevice->createCommandBuffer(
        VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

    VkViewport viewport =
        vks::initializers::viewport((float)dim, (float)dim, 0.0f, 1.0f);
    VkRect2D scissor = vks::initializers::rect2D(dim, dim, 0, 0);

    vkCmdSetViewport(cmdBuf, 0, 1, &viewport);
    vkCmdSetScissor(cmdBuf, 0, 1, &scissor);

    VkImageSubresourceRange subresourceRange = {};
    subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.baseMipLevel = 0;
    subresourceRange.levelCount = numMips;
    subresourceRange.layerCount = 6;

    // Change image layout for all cubemap faces to transfer destination
    vks::tools::setImageLayout(
        cmdBuf, textures.irradianceCube.image, VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subresourceRange);

    for (uint32_t m = 0; m < numMips; m++) {
      for (uint32_t f = 0; f < 6; f++) {
        viewport.width = static_cast<float>(dim * std::pow(0.5f, m));
        viewport.height = static_cast<float>(dim * std::pow(0.5f, m));
        vkCmdSetViewport(cmdBuf, 0, 1, &viewport);

        // Render scene from cube face's point of view
        vkCmdBeginRenderPass(cmdBuf, &renderPassBeginInfo,
                             VK_SUBPASS_CONTENTS_INLINE);

        // Update shader push constant block
        pushBlock.mvp =
            glm::perspective((float)(M_PI / 2.0), 1.0f, 0.1f, 512.0f) *
            matrices[f];

        vkCmdPushConstants(
            cmdBuf, pipelinelayout,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
            sizeof(PushBlock), &pushBlock);

        vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
        vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                pipelinelayout, 0, 1, &descriptorset, 0, NULL);

        models.skybox.draw(cmdBuf);

        vkCmdEndRenderPass(cmdBuf);

        vks::tools::setImageLayout(cmdBuf, offscreen.image,
                                   VK_IMAGE_ASPECT_COLOR_BIT,
                                   VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                   VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

        // Copy region for transfer from framebuffer to cube face
        VkImageCopy copyRegion = {};

        copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copyRegion.srcSubresource.baseArrayLayer = 0;
        copyRegion.srcSubresource.mipLevel = 0;
        copyRegion.srcSubresource.layerCount = 1;
        copyRegion.srcOffset = {0, 0, 0};

        copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copyRegion.dstSubresource.baseArrayLayer = f;
        copyRegion.dstSubresource.mipLevel = m;
        copyRegion.dstSubresource.layerCount = 1;
        copyRegion.dstOffset = {0, 0, 0};

        copyRegion.extent.width = static_cast<uint32_t>(viewport.width);
        copyRegion.extent.height = static_cast<uint32_t>(viewport.height);
        copyRegion.extent.depth = 1;

        vkCmdCopyImage(cmdBuf, offscreen.image,
                       VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                       textures.irradianceCube.image,
                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

        // Transform framebuffer color attachment back
        vks::tools::setImageLayout(cmdBuf, offscreen.image,
                                   VK_IMAGE_ASPECT_COLOR_BIT,
                                   VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                   VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
      }
    }

    vks::tools::setImageLayout(cmdBuf, textures.irradianceCube.image,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                               VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                               subresourceRange);

    vulkanDevice->flushCommandBuffer(cmdBuf, graphicsQueue);

    vkDestroyRenderPass(device, renderpass, nullptr);
    vkDestroyFramebuffer(device, offscreen.framebuffer, nullptr);
    vkFreeMemory(device, offscreen.memory, nullptr);
    vkDestroyImageView(device, offscreen.view, nullptr);
    vkDestroyImage(device, offscreen.image, nullptr);
    vkDestroyDescriptorPool(device, descriptorpool, nullptr);
    vkDestroyDescriptorSetLayout(device, descriptorsetlayout, nullptr);
    vkDestroyPipeline(device, pipeline, nullptr);
    vkDestroyPipelineLayout(device, pipelinelayout, nullptr);

    auto tEnd = std::chrono::high_resolution_clock::now();
    auto tDiff =
        std::chrono::duration<double, std::milli>(tEnd - tStart).count();
    std::cout << "Generating irradiance cube with " << numMips
              << " mip levels took " << tDiff << " ms" << std::endl;
  }

  // Prefilter environment cubemap
  void generatePrefilteredCube() {
    auto tStart = std::chrono::high_resolution_clock::now();

    const VkFormat format = VK_FORMAT_R16G16B16A16_SFLOAT;
    const int32_t dim = 512;
    const uint32_t numMips = static_cast<uint32_t>(floor(log2(dim))) + 1;

    // Pre-filtered cube map
    // Image
    VkImageCreateInfo imageCI = vks::initializers::imageCreateInfo();
    imageCI.imageType = VK_IMAGE_TYPE_2D;
    imageCI.format = format;
    imageCI.extent.width = dim;
    imageCI.extent.height = dim;
    imageCI.extent.depth = 1;
    imageCI.mipLevels = numMips;
    imageCI.arrayLayers = 6;
    imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCI.usage =
        VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    imageCI.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    VK_CHECK_RESULT(vkCreateImage(device, &imageCI, nullptr,
                                  &textures.prefilteredCube.image));
    VkMemoryAllocateInfo memAlloc = vks::initializers::memoryAllocateInfo();
    VkMemoryRequirements memReqs;
    vkGetImageMemoryRequirements(device, textures.prefilteredCube.image,
                                 &memReqs);
    memAlloc.allocationSize = memReqs.size;
    memAlloc.memoryTypeIndex = vulkanDevice->getMemoryType(
        memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    VK_CHECK_RESULT(vkAllocateMemory(device, &memAlloc, nullptr,
                                     &textures.prefilteredCube.deviceMemory));
    VK_CHECK_RESULT(vkBindImageMemory(device, textures.prefilteredCube.image,
                                      textures.prefilteredCube.deviceMemory,
                                      0));
    // Image view
    VkImageViewCreateInfo viewCI = vks::initializers::imageViewCreateInfo();
    viewCI.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
    viewCI.format = format;
    viewCI.subresourceRange = {};
    viewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewCI.subresourceRange.levelCount = numMips;
    viewCI.subresourceRange.layerCount = 6;
    viewCI.image = textures.prefilteredCube.image;
    VK_CHECK_RESULT(vkCreateImageView(device, &viewCI, nullptr,
                                      &textures.prefilteredCube.view));
    // Sampler
    VkSamplerCreateInfo samplerCI = vks::initializers::samplerCreateInfo();
    samplerCI.magFilter = VK_FILTER_LINEAR;
    samplerCI.minFilter = VK_FILTER_LINEAR;
    samplerCI.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerCI.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerCI.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerCI.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerCI.minLod = 0.0f;
    samplerCI.maxLod = static_cast<float>(numMips);
    samplerCI.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    VK_CHECK_RESULT(vkCreateSampler(device, &samplerCI, nullptr,
                                    &textures.prefilteredCube.sampler));

    textures.prefilteredCube.descriptor.imageView =
        textures.prefilteredCube.view;
    textures.prefilteredCube.descriptor.sampler =
        textures.prefilteredCube.sampler;
    textures.prefilteredCube.descriptor.imageLayout =
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    textures.prefilteredCube.device = vulkanDevice;

    VkAttachmentDescription attDesc = {};
    // Color attachment
    attDesc.format = format;
    attDesc.samples = VK_SAMPLE_COUNT_1_BIT;
    attDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attDesc.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    VkAttachmentReference colorReference = {
        0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

    VkSubpassDescription subpassDescription = {};
    subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescription.colorAttachmentCount = 1;
    subpassDescription.pColorAttachments = &colorReference;

    // Use subpass dependencies for layout transitions
    std::array<VkSubpassDependency, 2> dependencies;
    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[0].dstStageMask =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                                    VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    dependencies[1].srcSubpass = 0;
    dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                                    VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    // Renderpass
    VkRenderPassCreateInfo renderPassCI =
        vks::initializers::renderPassCreateInfo();
    renderPassCI.attachmentCount = 1;
    renderPassCI.pAttachments = &attDesc;
    renderPassCI.subpassCount = 1;
    renderPassCI.pSubpasses = &subpassDescription;
    renderPassCI.dependencyCount = 2;
    renderPassCI.pDependencies = dependencies.data();
    VkRenderPass renderpass;
    VK_CHECK_RESULT(
        vkCreateRenderPass(device, &renderPassCI, nullptr, &renderpass));

    struct {
      VkImage image;
      VkImageView view;
      VkDeviceMemory memory;
      VkFramebuffer framebuffer;
    } offscreen;

    // Offscreen framebuffer
    {
      // Color attachment
      VkImageCreateInfo imageCreateInfo = vks::initializers::imageCreateInfo();
      imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
      imageCreateInfo.format = format;
      imageCreateInfo.extent.width = dim;
      imageCreateInfo.extent.height = dim;
      imageCreateInfo.extent.depth = 1;
      imageCreateInfo.mipLevels = 1;
      imageCreateInfo.arrayLayers = 1;
      imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
      imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
      imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      imageCreateInfo.usage =
          VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
      imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
      VK_CHECK_RESULT(
          vkCreateImage(device, &imageCreateInfo, nullptr, &offscreen.image));

      VkMemoryAllocateInfo memAlloc = vks::initializers::memoryAllocateInfo();
      VkMemoryRequirements memReqs;
      vkGetImageMemoryRequirements(device, offscreen.image, &memReqs);
      memAlloc.allocationSize = memReqs.size;
      memAlloc.memoryTypeIndex = vulkanDevice->getMemoryType(
          memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
      VK_CHECK_RESULT(
          vkAllocateMemory(device, &memAlloc, nullptr, &offscreen.memory));
      VK_CHECK_RESULT(
          vkBindImageMemory(device, offscreen.image, offscreen.memory, 0));

      VkImageViewCreateInfo colorImageView =
          vks::initializers::imageViewCreateInfo();
      colorImageView.viewType = VK_IMAGE_VIEW_TYPE_2D;
      colorImageView.format = format;
      colorImageView.flags = 0;
      colorImageView.subresourceRange = {};
      colorImageView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      colorImageView.subresourceRange.baseMipLevel = 0;
      colorImageView.subresourceRange.levelCount = 1;
      colorImageView.subresourceRange.baseArrayLayer = 0;
      colorImageView.subresourceRange.layerCount = 1;
      colorImageView.image = offscreen.image;
      VK_CHECK_RESULT(
          vkCreateImageView(device, &colorImageView, nullptr, &offscreen.view));

      VkFramebufferCreateInfo fbufCreateInfo =
          vks::initializers::framebufferCreateInfo();
      fbufCreateInfo.renderPass = renderpass;
      fbufCreateInfo.attachmentCount = 1;
      fbufCreateInfo.pAttachments = &offscreen.view;
      fbufCreateInfo.width = dim;
      fbufCreateInfo.height = dim;
      fbufCreateInfo.layers = 1;
      VK_CHECK_RESULT(vkCreateFramebuffer(device, &fbufCreateInfo, nullptr,
                                          &offscreen.framebuffer));

      VkCommandBuffer layoutCmd = vulkanDevice->createCommandBuffer(
          VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
      vks::tools::setImageLayout(
          layoutCmd, offscreen.image, VK_IMAGE_ASPECT_COLOR_BIT,
          VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
      vulkanDevice->flushCommandBuffer(layoutCmd, graphicsQueue, true);
    }

    // Descriptors
    VkDescriptorSetLayout descriptorsetlayout;
    std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
        vks::initializers::descriptorSetLayoutBinding(
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            VK_SHADER_STAGE_FRAGMENT_BIT, 0),
    };
    VkDescriptorSetLayoutCreateInfo descriptorsetlayoutCI =
        vks::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings);
    VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorsetlayoutCI,
                                                nullptr, &descriptorsetlayout));

    // Descriptor Pool
    std::vector<VkDescriptorPoolSize> poolSizes = {
        vks::initializers::descriptorPoolSize(
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1)};
    VkDescriptorPoolCreateInfo descriptorPoolCI =
        vks::initializers::descriptorPoolCreateInfo(poolSizes, 2);
    VkDescriptorPool descriptorpool;
    VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolCI, nullptr,
                                           &descriptorpool));

    // Descriptor sets
    VkDescriptorSet descriptorset;
    VkDescriptorSetAllocateInfo allocInfo =
        vks::initializers::descriptorSetAllocateInfo(descriptorpool,
                                                     &descriptorsetlayout, 1);
    VK_CHECK_RESULT(
        vkAllocateDescriptorSets(device, &allocInfo, &descriptorset));
    VkWriteDescriptorSet writeDescriptorSet =
        vks::initializers::writeDescriptorSet(
            descriptorset, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0,
            &textures.environmentCube.descriptor);
    vkUpdateDescriptorSets(device, 1, &writeDescriptorSet, 0, nullptr);

    // Pipeline layout
    struct PushBlock {
      glm::mat4 mvp;
      float roughness;
      uint32_t numSamples = 32u;
    } pushBlock;

    VkPipelineLayout pipelinelayout;
    std::vector<VkPushConstantRange> pushConstantRanges = {
        vks::initializers::pushConstantRange(
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            sizeof(PushBlock), 0),
    };
    VkPipelineLayoutCreateInfo pipelineLayoutCI =
        vks::initializers::pipelineLayoutCreateInfo(&descriptorsetlayout, 1);
    pipelineLayoutCI.pushConstantRangeCount = 1;
    pipelineLayoutCI.pPushConstantRanges = pushConstantRanges.data();
    VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCI, nullptr,
                                           &pipelinelayout));

    // Pipeline
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyState =
        vks::initializers::pipelineInputAssemblyStateCreateInfo(
            VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
    VkPipelineRasterizationStateCreateInfo rasterizationState =
        vks::initializers::pipelineRasterizationStateCreateInfo(
            VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE,
            VK_FRONT_FACE_COUNTER_CLOCKWISE);
    VkPipelineColorBlendAttachmentState blendAttachmentState =
        vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE);
    VkPipelineColorBlendStateCreateInfo colorBlendState =
        vks::initializers::pipelineColorBlendStateCreateInfo(
            1, &blendAttachmentState);
    VkPipelineDepthStencilStateCreateInfo depthStencilState =
        vks::initializers::pipelineDepthStencilStateCreateInfo(
            VK_FALSE, VK_FALSE, VK_COMPARE_OP_LESS_OR_EQUAL);
    VkPipelineViewportStateCreateInfo viewportState =
        vks::initializers::pipelineViewportStateCreateInfo(1, 1);
    VkPipelineMultisampleStateCreateInfo multisampleState =
        vks::initializers::pipelineMultisampleStateCreateInfo(
            VK_SAMPLE_COUNT_1_BIT);
    std::vector<VkDynamicState> dynamicStateEnables = {
        VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicState =
        vks::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables);
    std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;

    VkGraphicsPipelineCreateInfo pipelineCI =
        vks::initializers::graphicsPipelineCreateInfo(pipelinelayout,
                                                      renderpass);
    pipelineCI.pInputAssemblyState = &inputAssemblyState;
    pipelineCI.pRasterizationState = &rasterizationState;
    pipelineCI.pColorBlendState = &colorBlendState;
    pipelineCI.pMultisampleState = &multisampleState;
    pipelineCI.pViewportState = &viewportState;
    pipelineCI.pDepthStencilState = &depthStencilState;
    pipelineCI.pDynamicState = &dynamicState;
    pipelineCI.stageCount = 2;
    pipelineCI.pStages = shaderStages.data();
    pipelineCI.renderPass = renderpass;
    pipelineCI.pVertexInputState = vkglTF::Vertex::getPipelineVertexInputState(
        {vkglTF::VertexComponent::Position, vkglTF::VertexComponent::Normal,
         vkglTF::VertexComponent::UV});

    shaderStages[0] =
        loadShader("shaders/filtercube.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
    shaderStages[1] = loadShader("shaders/prefilterenvmap.frag.spv",
                                 VK_SHADER_STAGE_FRAGMENT_BIT);
    VkPipeline pipeline;
    VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1,
                                              &pipelineCI, nullptr, &pipeline));

    // Render

    VkClearValue clearValues[1];
    clearValues[0].color = {{0.0f, 0.0f, 0.2f, 0.0f}};

    VkRenderPassBeginInfo renderPassBeginInfo =
        vks::initializers::renderPassBeginInfo();
    // Reuse render pass from example pass
    renderPassBeginInfo.renderPass = renderpass;
    renderPassBeginInfo.framebuffer = offscreen.framebuffer;
    renderPassBeginInfo.renderArea.extent.width = dim;
    renderPassBeginInfo.renderArea.extent.height = dim;
    renderPassBeginInfo.clearValueCount = 1;
    renderPassBeginInfo.pClearValues = clearValues;

    std::vector<glm::mat4> matrices = {
        // POSITIVE_X
        glm::rotate(glm::rotate(glm::mat4(1.0f), glm::radians(90.0f),
                                glm::vec3(0.0f, 1.0f, 0.0f)),
                    glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
        // NEGATIVE_X
        glm::rotate(glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f),
                                glm::vec3(0.0f, 1.0f, 0.0f)),
                    glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
        // POSITIVE_Y
        glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f),
                    glm::vec3(1.0f, 0.0f, 0.0f)),
        // NEGATIVE_Y
        glm::rotate(glm::mat4(1.0f), glm::radians(90.0f),
                    glm::vec3(1.0f, 0.0f, 0.0f)),
        // POSITIVE_Z
        glm::rotate(glm::mat4(1.0f), glm::radians(180.0f),
                    glm::vec3(1.0f, 0.0f, 0.0f)),
        // NEGATIVE_Z
        glm::rotate(glm::mat4(1.0f), glm::radians(180.0f),
                    glm::vec3(0.0f, 0.0f, 1.0f)),
    };

    VkCommandBuffer cmdBuf = vulkanDevice->createCommandBuffer(
        VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

    VkViewport viewport =
        vks::initializers::viewport((float)dim, (float)dim, 0.0f, 1.0f);
    VkRect2D scissor = vks::initializers::rect2D(dim, dim, 0, 0);

    vkCmdSetViewport(cmdBuf, 0, 1, &viewport);
    vkCmdSetScissor(cmdBuf, 0, 1, &scissor);

    VkImageSubresourceRange subresourceRange = {};
    subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.baseMipLevel = 0;
    subresourceRange.levelCount = numMips;
    subresourceRange.layerCount = 6;

    // Change image layout for all cubemap faces to transfer destination
    vks::tools::setImageLayout(
        cmdBuf, textures.prefilteredCube.image, VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subresourceRange);

    for (uint32_t m = 0; m < numMips; m++) {
      pushBlock.roughness = (float)m / (float)(numMips - 1);
      for (uint32_t f = 0; f < 6; f++) {
        viewport.width = static_cast<float>(dim * std::pow(0.5f, m));
        viewport.height = static_cast<float>(dim * std::pow(0.5f, m));
        vkCmdSetViewport(cmdBuf, 0, 1, &viewport);

        // Render scene from cube face's point of view
        vkCmdBeginRenderPass(cmdBuf, &renderPassBeginInfo,
                             VK_SUBPASS_CONTENTS_INLINE);

        // Update shader push constant block
        pushBlock.mvp =
            glm::perspective((float)(M_PI / 2.0), 1.0f, 0.1f, 512.0f) *
            matrices[f];

        vkCmdPushConstants(
            cmdBuf, pipelinelayout,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
            sizeof(PushBlock), &pushBlock);

        vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
        vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                pipelinelayout, 0, 1, &descriptorset, 0, NULL);

        models.skybox.draw(cmdBuf);

        vkCmdEndRenderPass(cmdBuf);

        vks::tools::setImageLayout(cmdBuf, offscreen.image,
                                   VK_IMAGE_ASPECT_COLOR_BIT,
                                   VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                   VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

        // Copy region for transfer from framebuffer to cube face
        VkImageCopy copyRegion = {};

        copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copyRegion.srcSubresource.baseArrayLayer = 0;
        copyRegion.srcSubresource.mipLevel = 0;
        copyRegion.srcSubresource.layerCount = 1;
        copyRegion.srcOffset = {0, 0, 0};

        copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copyRegion.dstSubresource.baseArrayLayer = f;
        copyRegion.dstSubresource.mipLevel = m;
        copyRegion.dstSubresource.layerCount = 1;
        copyRegion.dstOffset = {0, 0, 0};

        copyRegion.extent.width = static_cast<uint32_t>(viewport.width);
        copyRegion.extent.height = static_cast<uint32_t>(viewport.height);
        copyRegion.extent.depth = 1;

        vkCmdCopyImage(cmdBuf, offscreen.image,
                       VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                       textures.prefilteredCube.image,
                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

        // Transform framebuffer color attachment back
        vks::tools::setImageLayout(cmdBuf, offscreen.image,
                                   VK_IMAGE_ASPECT_COLOR_BIT,
                                   VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                   VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
      }
    }

    vks::tools::setImageLayout(cmdBuf, textures.prefilteredCube.image,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                               VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                               subresourceRange);

    vulkanDevice->flushCommandBuffer(cmdBuf, graphicsQueue);

    vkDestroyRenderPass(device, renderpass, nullptr);
    vkDestroyFramebuffer(device, offscreen.framebuffer, nullptr);
    vkFreeMemory(device, offscreen.memory, nullptr);
    vkDestroyImageView(device, offscreen.view, nullptr);
    vkDestroyImage(device, offscreen.image, nullptr);
    vkDestroyDescriptorPool(device, descriptorpool, nullptr);
    vkDestroyDescriptorSetLayout(device, descriptorsetlayout, nullptr);
    vkDestroyPipeline(device, pipeline, nullptr);
    vkDestroyPipelineLayout(device, pipelinelayout, nullptr);

    auto tEnd = std::chrono::high_resolution_clock::now();
    auto tDiff =
        std::chrono::duration<double, std::milli>(tEnd - tStart).count();
    std::cout << "Generating pre-filtered enivornment cube with " << numMips
              << " mip levels took " << tDiff << " ms" << std::endl;
  }

  void prepareUniformBuffers() {
    // Object vertex shader uniform buffer
    VK_CHECK_RESULT(vulkanDevice->createBuffer(
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &uniformBuffers.object, sizeof(uboMatrices)));

    // Skybox vertex shader uniform buffer
    VK_CHECK_RESULT(vulkanDevice->createBuffer(
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &uniformBuffers.skybox, sizeof(uboMatrices)));

    // Shared parameter uniform buffer
    VK_CHECK_RESULT(
        vulkanDevice->createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                       VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                   &uniformBuffers.params, sizeof(uboParams)));

    // Post Processing Vertex Uniform buffer
    VK_CHECK_RESULT(vulkanDevice->createBuffer(
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &uniformBuffers.postProcessing, sizeof(PostProcessingParams),
        &postProcessingParams));

    // Map persistent
    VK_CHECK_RESULT(uniformBuffers.object.map());
    VK_CHECK_RESULT(uniformBuffers.skybox.map());
    VK_CHECK_RESULT(uniformBuffers.params.map());
    VK_CHECK_RESULT(uniformBuffers.postProcessing.map());

    updateUniformBuffers();
    updateParams();
  }

  void updateUniformBuffers() {
    // 3D object
    uboMatrices.projection = camera.matrices.perspective;
    uboMatrices.view = camera.matrices.view;
    uboMatrices.model = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f),
                                    glm::vec3(0.0f, 1.0f, 0.0f));
    uboMatrices.camPos = camera.position * -1.0f;
    memcpy(uniformBuffers.object.mapped, &uboMatrices, sizeof(uboMatrices));

    // Skybox
    glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(5.0f, 5.0f, 5.0f));
    uboMatrices.model = glm::mat4(glm::mat3(camera.matrices.view)) * scale;
    memcpy(uniformBuffers.skybox.mapped, &uboMatrices, sizeof(uboMatrices));
  }

  void updateParams() {
    memcpy(uniformBuffers.params.mapped, &uboParams, sizeof(uboParams));
    memcpy(uniformBuffers.postProcessing.mapped, &postProcessingParams,
           sizeof(PostProcessingParams));
  }

  void loadAssets() {
    const uint32_t glTFLoadingFlags =
        vkglTF::FileLoadingFlags::PreTransformVertices |
        vkglTF::FileLoadingFlags::PreMultiplyVertexColors |
        vkglTF::FileLoadingFlags::FlipY;

    models.skybox.loadFromFile(getAssetPath() + "models/cube.gltf",
                               vulkanDevice, graphicsQueue, glTFLoadingFlags);
    models.cerberus.loadFromFile(
        getAssetPath() + "models/cerberus/cerberus.gltf", vulkanDevice,
        graphicsQueue, glTFLoadingFlags);
    textures.environmentCube.loadFromFile(
        getAssetPath() + "textures/hdr/pisa_cube.ktx",
        VK_FORMAT_R16G16B16A16_SFLOAT, vulkanDevice, graphicsQueue);
    textures.pbrTextures.albedoMap.loadFromFile(
        getAssetPath() + "models/cerberus/albedo.ktx", VK_FORMAT_R8G8B8A8_UNORM,
        vulkanDevice, graphicsQueue);
    textures.pbrTextures.normalMap.loadFromFile(
        getAssetPath() + "models/cerberus/normal.ktx", VK_FORMAT_R8G8B8A8_UNORM,
        vulkanDevice, graphicsQueue);
    textures.pbrTextures.aoMap.loadFromFile(
        getAssetPath() + "models/cerberus/ao.ktx", VK_FORMAT_R8_UNORM,
        vulkanDevice, graphicsQueue);
    textures.pbrTextures.metallicMap.loadFromFile(
        getAssetPath() + "models/cerberus/metallic.ktx", VK_FORMAT_R8_UNORM,
        vulkanDevice, graphicsQueue);
    textures.pbrTextures.roughnessMap.loadFromFile(
        getAssetPath() + "models/cerberus/roughness.ktx", VK_FORMAT_R8_UNORM,
        vulkanDevice, graphicsQueue);
  }

  void prepareImGui() {
    imGui = new vkImGUI(this);
    imGui->init((float)getWidth(), (float)getHeight());
    imGui->initResources(this, graphicsQueue, getShaderBasePath());
  }

  void prepare() override {
    BaseRenderer::prepare();
    loadAssets();
    generateBRDFLUT();
    generateIrradianceCube();
    generatePrefilteredCube();
    prepareUniformBuffers();
    setupDescriptors();
    preparePipelines();
    prepareImGui();
    prepared = true;
  }

  void draw() {
    BaseRenderer::prepareFrame();
    buildCommandBuffer();
    BaseRenderer::submitFrame();
  }

  void render() override {
    if (!prepared) return;

    updateUniformBuffers();

    // Update imGui
    ImGuiIO& io = ImGui::GetIO();

    io.DisplaySize = ImVec2((float)getWidth(), (float)getHeight());
    io.DeltaTime = frameTimer;

    io.MousePos = ImVec2(mouseState.position.x, mouseState.position.y);
    io.MouseDown[0] = mouseState.buttons.left && uiSettings.visible;
    io.MouseDown[1] = mouseState.buttons.right && uiSettings.visible;
    io.MouseDown[2] = mouseState.buttons.middle && uiSettings.visible;

    draw();
  }
};