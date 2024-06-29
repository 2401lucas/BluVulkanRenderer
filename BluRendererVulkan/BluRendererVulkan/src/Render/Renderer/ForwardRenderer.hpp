#pragma once
#define NOMINMAX

#include <corecrt_math_defines.h>

#include "../ResourceManagement/ExternalResources/ThreadPool.hpp"
#include "../ResourceManagement/ExternalResources/VulkanTexture.hpp"
#include "../ResourceManagement/ExternalResources/VulkanglTFModel.h"
#include "BaseRenderer.h"
#include "PostProcessingPass.h"
#include "vkImGui.h"

struct UISettings {
  bool visible = true;
  float scale = 1;
  std::array<float, 50> frameTimes{};
  float frameTimeMin = 9999.0f, frameTimeMax = 0.0f;
  // Scene Settings-----------------------------
  bool displaySkybox = true;
  glm::vec4 skyboxColor = glm::vec4(1.0f);
  bool displayScene = true;
  int activeSceneIndex = 0;
  int aaMode = 0;
  float IBLstrength = 1;
  int debugOutput = 0;
  int debugLight = 0;
  bool animateLight = true;
  // bool usePcfFiltering = false;
} uiSettings;

class ForwardRenderer : public BaseRenderer {
 public:
  vkImGUI* imGui = nullptr;

#ifndef SceneResources 1
  const char* scenes[3] = {"Sponza", "Roughness/Metallic", "Emissive Test"};
  const char* sceneFilePaths[3] = {
      "models/Sponza/Sponza.glb",
      "models/MetalRoughSpheres/MetalRoughSpheres.glb",
      "models/EmissiveStrengthTest/EmissiveStrengthTest.glb"};

  enum PBRWorkflows {
    PBR_WORKFLOW_METALLIC_ROUGHNESS = 0,
    PBR_WORKFLOW_SPECULAR_GLOSSINESS = 1
  };

  struct Textures {
    vks::TextureCubeMap environmentCube;
    vks::Texture2D empty;
    // Generated at runtime
    vks::Texture2D lutBrdf;
    vks::TextureCubeMap irradianceCube;
    vks::TextureCubeMap prefilteredCube;
  } textures;

  vkglTF::Model skybox;
  std::vector<vkglTF::Model> staticModels;
  std::vector<vkglTF::Model> dynamicModels;
  std::vector<uint32_t> staticModelsToRenderIndices;
  std::vector<uint32_t> dynamicModelsToRenderIndices;

  // We use a material buffer to pass material data ind image indices to the
  // shader
  struct alignas(16) ShaderMaterial {
    glm::vec4 baseColorFactor;
    glm::vec4 emissiveFactor;
    glm::vec4 diffuseFactor;
    glm::vec4 specularFactor;
    float workflow;
    int colorTextureSet;
    int PhysicalDescriptorTextureSet;
    int normalTextureSet;
    int occlusionTextureSet;
    int emissiveTextureSet;
    float metallicFactor;
    float roughnessFactor;
    float alphaMask;
    float alphaMaskCutoff;
    float emissiveStrength;
  };

  // TODO: MOVE TO MODEL
  int32_t animationIndex = 0;
  float animationTimer = 0.0f;
  bool animate = true;
#endif

#ifndef VulkanResources 1
  struct PushConstData {
    uint32_t materialIndex = 0;
    uint32_t transformMatIndex = 0;
  };
  struct DynamicDescriptorSets {
    VkDescriptorSet scene{VK_NULL_HANDLE};
    VkDescriptorSet skybox{VK_NULL_HANDLE};
    VkDescriptorSet shadow{VK_NULL_HANDLE};
  };

  struct DynamicUniformBuffers {
    vks::Buffer scene;
    vks::Buffer params;
    vks::Buffer shadow;
  };

  std::vector<DynamicDescriptorSets> dynamicDescriptorSets;
  std::vector<DynamicUniformBuffers> dynamicUniformBuffers;

  struct StaticDescriptorSets {
    VkDescriptorSet materials{VK_NULL_HANDLE};
  } staticDescriptorSets;

  struct StaticUniformBuffers {
    vks::Buffer postProcessing;
  } staticUniformBuffers;

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
    VkRenderPass renderPass{VK_NULL_HANDLE};
    VkSampler sampler{VK_NULL_HANDLE};
    std::vector<FrameBuffer> framebuffers;
  };

  struct {
    VkPipelineLayout scene{VK_NULL_HANDLE};
    VkPipelineLayout skybox{VK_NULL_HANDLE};
    VkPipelineLayout postProcessing{VK_NULL_HANDLE};
    VkPipelineLayout shadow{VK_NULL_HANDLE};
    VkPipelineLayout computeParticles{VK_NULL_HANDLE};
  } pipelineLayouts;

  struct {
    VkPipeline skybox{VK_NULL_HANDLE};
    VkPipeline postProcessing{VK_NULL_HANDLE};
    VkPipeline shadow{VK_NULL_HANDLE};
    VkPipeline computeParticles{VK_NULL_HANDLE};
  } pipelines;

  std::unordered_map<std::string, VkPipeline> genPipelines;
  VkPipeline boundPipeline{VK_NULL_HANDLE};

  struct {
    VkDescriptorSetLayout scene{VK_NULL_HANDLE};
    VkDescriptorSetLayout material{VK_NULL_HANDLE};
    VkDescriptorSetLayout node{VK_NULL_HANDLE};
    VkDescriptorSetLayout materialBuffer{VK_NULL_HANDLE};
    VkDescriptorSetLayout skybox{VK_NULL_HANDLE};
    VkDescriptorSetLayout postProcessing{VK_NULL_HANDLE};
    VkDescriptorSetLayout shadow{VK_NULL_HANDLE};
  } descriptorSetLayouts;

  std::vector<vks::PostProcessingPass*> postPasses;
  OffscreenPass offscreenShadowPass;

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

  struct {
    std::vector<VkCommandBuffer> ui;
    std::vector<VkCommandBuffer> postProcessing;
    std::vector<VkCommandBuffer> scene;
    std::vector<VkCommandBuffer> shadow;
    std::vector<VkCommandBuffer> compute;
  } commandBuffers;

  VkExtent2D attachmentSize{};

  const std::vector<std::string> supportedExtensions = {
      "KHR_texture_basisu", "KHR_materials_pbrSpecularGlossiness",
      "KHR_materials_unlit", "KHR_materials_emissive_strength"};

  uint32_t numThreads;
  vks::ThreadPool* threadPool;
#endif

#ifndef RenderSettings 1
#define MAX_MODELS 16
#define MAX_LIGHTS 2
  const uint32_t shadowMapSize = 2048;
  // Depth bias (and slope) are used to avoid shadowing artifacts
  const float depthBiasConstant = 1.25f;
  const float depthBiasSlope = 1.75f;

  const char* antiAliasingSettings[3] = {"Off", "FXAA", "TAA"};
  const char* aoSettings[3] = {"Off", "SSAO", "HBAO"};
  // NOTE FOR THE BUFFERS, NOT ALL BUFFERS NEED TO BE UPDATE PER FRAME, BUT ALL
  // THE BUFFERS NEED INITIAL UPDATE, FOR EVERY FRAME: THIS SHOULD BE CHANGED TO
  // 1 SHARED BUFFER AS UPDATES ARE RARE AND SHARED BETWEEN FRAMES
  struct PostProcessingParams {
    float aaType = 0;
    float aoType = 0;
    bool enableBloom = true;
  } postProcessingParams;

  struct ShadowParams {
    glm::mat4 depthMVP;
  } shadowParams;

  // Should (M?)VP be precomputed
  struct UBOMatrices {
    glm::mat4 models[MAX_MODELS];
    glm::mat4 projection;
    glm::mat4 view;
    glm::mat4 lightSpace;
    glm::vec3 camPos;
  } uboMatrices;

  const char* debugLights[2] = {"Image Baked Light", "Area Light"};

  struct LightSource {
    glm::vec4 color = glm::vec4(1.0f);
    glm::vec4 position = glm::vec4(0.0f, 20.0f, 0.0f, 0.0f);
    glm::vec4 rotation = glm::vec4(0.0f, -1.0f, 0.0f, 0.0f);
    // Keep depth range as small as possible for better shadow map precision
    float zNear = 20.0f;
    float zFar = 70.0f;
    float lightFOV = 45.0f;
    float placeholderSpace;
  };

  const char* debugInputs[12] = {"None", "Color Map", "Normals", "AO Map", "Emissive Map", "Metallic Map", "Roughness Map", "F", "G", "D", "IBL Contribution",
                                   "Light Contribution" /*, "Diffuse Contribution",
                                   "Specular Contribution"*/};
  struct SceneParams {
    LightSource lights[MAX_LIGHTS];
    float prefilteredCubeMipLevels;
    float debugViewInputs = 0.0f;
    float debugViewEquation = 0.0f;
    float debugViewLight = 0.0f;
  } sceneParams;

  glm::vec3 iblDir = glm::vec3(0.0f, -40.0f, 0.0f);
#endif

  ForwardRenderer() : BaseRenderer() {
    name = "Forward Renderer with PBR";
    camera.type = Camera::firstperson;
    camera.movementSpeed = 2.0f;
    camera.rotationSpeed = 0.25f;
    camera.setPosition(glm::vec3(-0.318f, 0.240f, -0.639f));
    camera.setRotation(glm::vec3(4.5f, -300.25f, 0.0f));
    camera.setPerspective(60.0f, (float)getWidth() / (float)getHeight(), 0.01f,
                          5000.0f);

    enabledInstanceExtensions.push_back(
        VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

    settings.overlay = true;
    settings.validation = true;

    threadPool = new vks::ThreadPool();
    // Get number of max. concurrent threads
    numThreads = std::thread::hardware_concurrency();
    assert(numThreads > 0);
    std::cout << "Number of Threads: " << numThreads << std::endl;
    threadPool->setThreadCount(numThreads);
  }

  ~ForwardRenderer() {
    for (auto& pipeline : genPipelines) {
      vkDestroyPipeline(device, pipeline.second, nullptr);
    }

    vkDestroyPipeline(device, pipelines.skybox, nullptr);
    vkDestroyPipeline(device, pipelines.postProcessing, nullptr);
    vkDestroyPipeline(device, pipelines.shadow, nullptr);

    vkDestroyPipelineLayout(device, pipelineLayouts.scene, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayouts.skybox, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayouts.postProcessing, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayouts.shadow, nullptr);
    vkDestroyDescriptorSetLayout(device, descriptorSetLayouts.scene, nullptr);
    vkDestroyDescriptorSetLayout(device, descriptorSetLayouts.material,
                                 nullptr);
    vkDestroyDescriptorSetLayout(device, descriptorSetLayouts.materialBuffer,
                                 nullptr);
    vkDestroyDescriptorSetLayout(device, descriptorSetLayouts.node, nullptr);
    vkDestroyDescriptorSetLayout(device, descriptorSetLayouts.skybox, nullptr);
    vkDestroyDescriptorSetLayout(device, descriptorSetLayouts.postProcessing,
                                 nullptr);
    vkDestroyDescriptorSetLayout(device, descriptorSetLayouts.shadow, nullptr);

    vkDestroyImage(device, multisampleTarget.color.image, nullptr);
    vkDestroyImageView(device, multisampleTarget.color.view, nullptr);
    vkFreeMemory(device, multisampleTarget.color.memory, nullptr);
    vkDestroyImage(device, multisampleTarget.depth.image, nullptr);
    vkDestroyImageView(device, multisampleTarget.depth.view, nullptr);
    vkFreeMemory(device, multisampleTarget.depth.memory, nullptr);

    for (auto& pass : postPasses) {
      delete pass;
    }

    for (uint32_t i = 0; i < swapChain.imageCount; i++) {
      vkDestroyImage(device, offscreenShadowPass.framebuffers[i].depth.image,
                     nullptr);
      vkDestroyImageView(device, offscreenShadowPass.framebuffers[i].depth.view,
                         nullptr);
      vkFreeMemory(device, offscreenShadowPass.framebuffers[i].depth.memory,
                   nullptr);
      vkDestroyFramebuffer(
          device, offscreenShadowPass.framebuffers[i].framebuffer, nullptr);

      dynamicUniformBuffers[i].scene.destroy();
      dynamicUniformBuffers[i].params.destroy();
      dynamicUniformBuffers[i].shadow.destroy();
    }

    vkDestroyRenderPass(device, offscreenShadowPass.renderPass, nullptr);
    vkDestroySampler(device, offscreenShadowPass.sampler, nullptr);

    staticUniformBuffers.postProcessing.destroy();

    for (auto& model : dynamicModels) {
      model.destroy();
    }
    for (auto& model : staticModels) {
      model.destroy();
    }

    skybox.destroy();
    textures.empty.destroy();
    textures.environmentCube.destroy();
    textures.irradianceCube.destroy();
    textures.prefilteredCube.destroy();
    textures.lutBrdf.destroy();

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
      setupOffscreenShadowRenderPass();
    }
  }

  void setupFrameBuffer() override {
    // If the window has been resized, destroy resources
    if (attachmentSize.width != getWidth() ||
        attachmentSize.height != getHeight()) {
      attachmentSize = {getWidth(), getHeight()};
      for (size_t i = 0; i < offscreenShadowPass.framebuffers.size(); i++) {
        vkDestroyImage(device, offscreenShadowPass.framebuffers[i].depth.image,
                       nullptr);
        vkDestroyImageView(
            device, offscreenShadowPass.framebuffers[i].depth.view, nullptr);
        vkFreeMemory(device, offscreenShadowPass.framebuffers[i].depth.memory,
                     nullptr);
        vkDestroyFramebuffer(
            device, offscreenShadowPass.framebuffers[i].framebuffer, nullptr);
      }
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
      vkDeviceWaitIdle(device);
      setupOffscreenShadowPassResources();
      setupOffscreenShadowPass();
    }
  }

  void setupOffscreenShadowRenderPass() {
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
    dependencies[0].dstAccessMask =
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    dependencies[1].srcSubpass = 0;
    dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependencies[1].srcAccessMask =
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
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
                                       &offscreenShadowPass.renderPass));

    VkFilter shadowmap_filter =
        vks::tools::formatIsFilterable(physicalDevice, VK_FORMAT_D16_UNORM,
                                       VK_IMAGE_TILING_OPTIMAL)
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
    VK_CHECK_RESULT(vkCreateSampler(device, &samplerInfo, nullptr,
                                    &offscreenShadowPass.sampler));
  }

  void setupOffscreenShadowPassResources() {
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
    image.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
                  VK_IMAGE_USAGE_SAMPLED_BIT;

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

    offscreenShadowPass.framebuffers.resize(swapChain.imageCount);
    for (uint32_t i = 0; i < swapChain.imageCount; i++) {
      VK_CHECK_RESULT(
          vkCreateImage(device, &image, nullptr,
                        &offscreenShadowPass.framebuffers[i].depth.image));
      vkGetImageMemoryRequirements(
          device, offscreenShadowPass.framebuffers[i].depth.image, &memReqs);
      memAlloc.allocationSize = memReqs.size;
      memAlloc.memoryTypeIndex = vulkanDevice->getMemoryType(
          memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
      VK_CHECK_RESULT(
          vkAllocateMemory(device, &memAlloc, nullptr,
                           &offscreenShadowPass.framebuffers[i].depth.memory));
      VK_CHECK_RESULT(vkBindImageMemory(
          device, offscreenShadowPass.framebuffers[i].depth.image,
          offscreenShadowPass.framebuffers[i].depth.memory, 0));

      depthStencilView.image = offscreenShadowPass.framebuffers[i].depth.image;
      VK_CHECK_RESULT(
          vkCreateImageView(device, &depthStencilView, nullptr,
                            &offscreenShadowPass.framebuffers[i].depth.view));
    }
  }

  void setupOffscreenShadowPass() {
    for (uint32_t i = 0; i < swapChain.imageCount; i++) {
      VkImageView attachments[1];
      attachments[0] = offscreenShadowPass.framebuffers[i].depth.view;

      VkFramebufferCreateInfo fbufCreateInfo =
          vks::initializers::framebufferCreateInfo();
      fbufCreateInfo.renderPass = offscreenShadowPass.renderPass;
      fbufCreateInfo.attachmentCount = 1;
      fbufCreateInfo.pAttachments = attachments;
      fbufCreateInfo.width = shadowMapSize;
      fbufCreateInfo.height = shadowMapSize;
      fbufCreateInfo.layers = 1;

      VK_CHECK_RESULT(vkCreateFramebuffer(
          device, &fbufCreateInfo, nullptr,
          &offscreenShadowPass.framebuffers[i].framebuffer));

      // Fill a descriptor for later use in a descriptor set
      offscreenShadowPass.framebuffers[i].descriptor.imageLayout =
          VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
      offscreenShadowPass.framebuffers[i].descriptor.imageView =
          offscreenShadowPass.framebuffers[i].depth.view;
      offscreenShadowPass.framebuffers[i].descriptor.sampler =
          offscreenShadowPass.sampler;
    }
  }

  void createCommandBuffers() override {
    BaseRenderer::createCommandBuffers();
    commandBuffers.ui.resize(swapChain.imageCount);
    commandBuffers.postProcessing.resize(swapChain.imageCount);
    commandBuffers.scene.resize(swapChain.imageCount);
    commandBuffers.shadow.resize(swapChain.imageCount);
    computeCmdBuffers.resize(swapChain.imageCount);

    VkCommandBufferAllocateInfo secondaryGraphicsCmdBufAllocateInfo =
        vks::initializers::commandBufferAllocateInfo(
            graphicsCmdPool, VK_COMMAND_BUFFER_LEVEL_SECONDARY,
            static_cast<uint32_t>(commandBuffers.ui.size()));

    VK_CHECK_RESULT(
        vkAllocateCommandBuffers(device, &secondaryGraphicsCmdBufAllocateInfo,
                                 commandBuffers.ui.data()));
    VK_CHECK_RESULT(
        vkAllocateCommandBuffers(device, &secondaryGraphicsCmdBufAllocateInfo,
                                 commandBuffers.postProcessing.data()));
    VK_CHECK_RESULT(
        vkAllocateCommandBuffers(device, &secondaryGraphicsCmdBufAllocateInfo,
                                 commandBuffers.scene.data()));
    VK_CHECK_RESULT(
        vkAllocateCommandBuffers(device, &secondaryGraphicsCmdBufAllocateInfo,
                                 commandBuffers.shadow.data()));

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
                         static_cast<uint32_t>(commandBuffers.ui.size()),
                         commandBuffers.ui.data());
    vkFreeCommandBuffers(
        device, graphicsCmdPool,
        static_cast<uint32_t>(commandBuffers.postProcessing.size()),
        commandBuffers.postProcessing.data());
    vkFreeCommandBuffers(device, graphicsCmdPool,
                         static_cast<uint32_t>(commandBuffers.shadow.size()),
                         commandBuffers.shadow.data());
    vkFreeCommandBuffers(device, graphicsCmdPool,
                         static_cast<uint32_t>(commandBuffers.scene.size()),
                         commandBuffers.scene.data());
  }

  // Starts a new imGui frame and sets up windows and ui elements
  void newUIFrame(bool updateFrameGraph) {
    ImGui::NewFrame();

    // Debug window
    ImGui::SetWindowPos(ImVec2(20 * uiSettings.scale, 20 * uiSettings.scale),
                        ImGuiCond_FirstUseEver);
    ImGui::SetWindowSize(ImVec2(300 * uiSettings.scale, 185 * uiSettings.scale),
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

    ImGui::Text("Total Model Count: &i",
                (int)(staticModels.size() + dynamicModels.size()));
    ImGui::Text("Rendered Models: &i", (int)dynamicModelsToRenderIndices.size());

    ImGui::SetNextWindowPos(
        ImVec2(20 * uiSettings.scale, 360 * uiSettings.scale),
        ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(
        ImVec2(300 * uiSettings.scale, 200 * uiSettings.scale),
        ImGuiCond_FirstUseEver);
    ImGui::Begin("Settings");
    if (ImGui::CollapsingHeader("Scene Settings")) {
      if (ImGui::BeginCombo("Active Scene",
                            scenes[uiSettings.activeSceneIndex])) {
        const char* currentItem = scenes[uiSettings.activeSceneIndex];
        for (int n = 0; n < sizeof(scenes) / sizeof(scenes[0]); n++) {
          bool is_selected = (currentItem == scenes[n]);
          if (ImGui::Selectable(scenes[n], is_selected)) {
            uiSettings.activeSceneIndex = 0;
            // loadScene();
          }
          if (is_selected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
      }

      ImGui::Checkbox("Display Level", &uiSettings.displayScene);
      ImGui::Checkbox("Display Skybox", &uiSettings.displaySkybox);
      ImGui::ColorPicker3("Skybox Clear Color", &uiSettings.skyboxColor.x);
    }

    if (ImGui::CollapsingHeader("Light Settings")) {
      ImGui::DragFloat("IBL Strength", &uiSettings.IBLstrength, 0.1f, 0.0f,
                       2.0f);

      ImGui::Checkbox("Animate Light", &uiSettings.animateLight);
    }

    if (ImGui::CollapsingHeader("Rendering Settings")) {
      if (ImGui::BeginCombo("Target Light to Debug",
                            debugLights[uiSettings.debugLight])) {
        const char* currentItem = debugLights[uiSettings.debugLight];
        for (int n = 0; n < sizeof(debugLights) / sizeof(debugLights[0]); n++) {
          bool is_selected = (currentItem == debugLights[n]);
          if (ImGui::Selectable(debugLights[n], is_selected)) {
            uiSettings.debugLight = n;
          }
          if (is_selected)
            ImGui::SetItemDefaultFocus();  // You may set the initial focus
                                           // when opening the combo
                                           // (scrolling + for keyboard
                                           // navigation support)
        }
        ImGui::EndCombo();
      }

      if (ImGui::BeginCombo("Debug View Inputs",
                            debugInputs[uiSettings.debugOutput])) {
        const char* currentItem = debugInputs[uiSettings.debugOutput];
        for (int n = 0; n < sizeof(debugInputs) / sizeof(debugInputs[0]); n++) {
          bool is_selected = (currentItem == debugInputs[n]);
          if (ImGui::Selectable(debugInputs[n], is_selected)) {
            uiSettings.debugOutput = n;
          }
          if (is_selected)
            ImGui::SetItemDefaultFocus();  // You may set the initial focus
                                           // when opening the combo
                                           // (scrolling + for keyboard
                                           // navigation support)
        }
        ImGui::EndCombo();
      }

      if (ImGui::BeginCombo("Anti Aliasing Type",
                            antiAliasingSettings[uiSettings.aaMode])) {
        const char* currentItem = antiAliasingSettings[uiSettings.aaMode];
        for (int n = 0;
             n < sizeof(antiAliasingSettings) / sizeof(antiAliasingSettings[0]);
             n++) {
          bool is_selected = (currentItem == antiAliasingSettings[n]);
          if (ImGui::Selectable(antiAliasingSettings[n], is_selected)) {
            uiSettings.aaMode = n;
            updatePostProcessingParams();
          }
          if (is_selected)
            ImGui::SetItemDefaultFocus();  // You may set the initial focus
                                           // when opening the combo
                                           // (scrolling + for keyboard
                                           // navigation support)
        }
        ImGui::EndCombo();
      }
    }
    if (ImGui::CollapsingHeader("UI Settings")) {
      if (ImGui::BeginCombo("UI style", imGui->styles[imGui->selectedStyle])) {
        const char* currentItem = imGui->styles[imGui->selectedStyle];
        for (int n = 0; n < sizeof(imGui->styles) / sizeof(imGui->styles[0]);
             n++) {
          bool is_selected =
              (currentItem == imGui->styles[n]);  // You can store your
                                                  // selection however you want,
          // outside or inside your objects
          if (ImGui::Selectable(imGui->styles[n], is_selected))
            imGui->setStyle(n);
          if (is_selected)
            ImGui::SetItemDefaultFocus();  // You may set the initial focus
                                           // when opening the combo
                                           // (scrolling + for keyboard
                                           // navigation support)
        }
        ImGui::EndCombo();
      }
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
        commandBuffers.postProcessing[currentFrameIndex];
    vkResetCommandBuffer(currentCommandBuffer, 0);
    VK_CHECK_RESULT(vkBeginCommandBuffer(currentCommandBuffer, &cmdBufInfo));

    VkViewport viewport = vks::initializers::viewport(
        (float)getWidth(), (float)getHeight(), 0.0f, 1.0f);
    vkCmdSetViewport(currentCommandBuffer, 0, 1, &viewport);
    VkRect2D scissor = vks::initializers::rect2D(getWidth(), getHeight(), 0, 0);
    vkCmdSetScissor(currentCommandBuffer, 0, 1, &scissor);

    VkDeviceSize offsets[1] = {0};

    vkCmdBindDescriptorSets(
        currentCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipelineLayouts.postProcessing, 0, 1,
        &postPasses[postPasses.size() - 1]->descriptorSets[currentFrameIndex],
        0, NULL);
    vkCmdBindPipeline(currentCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                      postPasses[postPasses.size() - 1]->pipeline);

    // Draw triangle
    vkCmdDraw(currentCommandBuffer, 3, 1, 0, 0);

    VK_CHECK_RESULT(vkEndCommandBuffer(currentCommandBuffer));
  }

  void buildUICommandBuffer() {
    VkCommandBufferInheritanceInfo inheritanceInfo =
        vks::initializers::commandBufferInheritanceInfo();
    inheritanceInfo.renderPass = renderPass;
    inheritanceInfo.framebuffer = frameBuffers[currentFrameIndex];

    VkCommandBufferBeginInfo cmdBufInfo =
        vks::initializers::commandBufferBeginInfo();
    cmdBufInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
    cmdBufInfo.pInheritanceInfo = &inheritanceInfo;

    VkCommandBuffer currentCommandBuffer = commandBuffers.ui[currentFrameIndex];

    vkResetCommandBuffer(currentCommandBuffer, 0);
    VK_CHECK_RESULT(vkBeginCommandBuffer(currentCommandBuffer, &cmdBufInfo));

    imGui->drawFrame(currentCommandBuffer, currentFrameIndex);

    VK_CHECK_RESULT(vkEndCommandBuffer(currentCommandBuffer));
  }

  void buildSceneCommandBuffer() {
    VkCommandBufferInheritanceInfo inheritanceInfo =
        vks::initializers::commandBufferInheritanceInfo();
    inheritanceInfo.renderPass = postPasses[0]->renderPass;
    inheritanceInfo.framebuffer =
        postPasses[0]->framebuffers[currentFrameIndex].framebuffer;

    VkCommandBufferBeginInfo cmdBufInfo =
        vks::initializers::commandBufferBeginInfo();
    cmdBufInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
    cmdBufInfo.pInheritanceInfo = &inheritanceInfo;

    VkCommandBuffer currentCommandBuffer =
        commandBuffers.scene[currentFrameIndex];
    vkResetCommandBuffer(currentCommandBuffer, 0);
    VK_CHECK_RESULT(vkBeginCommandBuffer(currentCommandBuffer, &cmdBufInfo));

    VkViewport viewport = vks::initializers::viewport(
        (float)getWidth(), (float)getHeight(), 0.0f, 1.0f);
    vkCmdSetViewport(currentCommandBuffer, 0, 1, &viewport);
    VkRect2D scissor = vks::initializers::rect2D(getWidth(), getHeight(), 0, 0);
    vkCmdSetScissor(currentCommandBuffer, 0, 1, &scissor);

    VkDeviceSize offsets[1] = {0};

    for (uint32_t i = 0; i < dynamicModelsToRenderIndices.size(); i++) {
      vkglTF::Model& model = dynamicModels[dynamicModelsToRenderIndices[i]];

      vkCmdBindVertexBuffers(currentCommandBuffer, 0, 1, &model.vertices.buffer,
                             offsets);
      if (model.indices.buffer != VK_NULL_HANDLE) {
        vkCmdBindIndexBuffer(currentCommandBuffer, model.indices.buffer, 0,
                             VK_INDEX_TYPE_UINT32);
      }

      boundPipeline = VK_NULL_HANDLE;
      PushConstData pushConst{};
      pushConst.transformMatIndex = i + 1;

      // Opaque primitives first
      for (auto node : model.nodes) {
        renderNode(node, currentFrameIndex, vkglTF::Material::ALPHAMODE_OPAQUE,
                   currentCommandBuffer, pushConst);
      }
      // Alpha masked primitives
      for (auto node : model.nodes) {
        renderNode(node, currentFrameIndex, vkglTF::Material::ALPHAMODE_MASK,
                   currentCommandBuffer, pushConst);
      }
      // Transparent primitives
      // TODO: Correct depth sorting
      for (auto node : model.nodes) {
        renderNode(node, currentFrameIndex, vkglTF::Material::ALPHAMODE_BLEND,
                   currentCommandBuffer, pushConst);
      }
    }

    // Rendered last to use early Z buffer rejection
    if (uiSettings.displaySkybox) {
      vkCmdBindDescriptorSets(
          currentCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
          pipelineLayouts.skybox, 0, 1,
          &dynamicDescriptorSets[currentFrameIndex].skybox, 0, NULL);
      vkCmdBindPipeline(currentCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                        pipelines.skybox);

      PushConstData pushConst{};
      pushConst.transformMatIndex = 0;
      vkCmdPushConstants(
          currentCommandBuffer, pipelineLayouts.skybox,
          VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
          sizeof(PushConstData), &pushConst);
      skybox.draw(currentCommandBuffer);
    }

    VK_CHECK_RESULT(vkEndCommandBuffer(currentCommandBuffer));
  }

  // void buildShadowCommandBuffer() {
  //   VkCommandBufferInheritanceInfo inheritanceInfo =
  //       vks::initializers::commandBufferInheritanceInfo();
  //   inheritanceInfo.renderPass = offscreenShadowPass.renderPass;
  //   inheritanceInfo.framebuffer =
  //       offscreenShadowPass.framebuffers[currentFrameIndex].framebuffer;

  //  VkCommandBufferBeginInfo cmdBufInfo =
  //      vks::initializers::commandBufferBeginInfo();
  //  cmdBufInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
  //  cmdBufInfo.pInheritanceInfo = &inheritanceInfo;

  //  VkCommandBuffer currentCommandBuffer =
  //      commandBuffers.shadow[currentFrameIndex];

  //  vkResetCommandBuffer(currentCommandBuffer, 0);
  //  VK_CHECK_RESULT(vkBeginCommandBuffer(currentCommandBuffer, &cmdBufInfo));

  //  VkViewport viewport = vks::initializers::viewport(
  //      (float)shadowMapSize, (float)shadowMapSize, 0.0f, 1.0f);
  //  vkCmdSetViewport(currentCommandBuffer, 0, 1, &viewport);
  //  VkRect2D scissor =
  //      vks::initializers::rect2D(shadowMapSize, shadowMapSize, 0, 0);
  //  vkCmdSetScissor(currentCommandBuffer, 0, 1, &scissor);

  //  // Set depth bias (aka "Polygon offset")
  //  vkCmdSetDepthBias(currentCommandBuffer, depthBiasConstant, 0.0f,
  //                    depthBiasSlope);
  //  VkDeviceSize offsets[1] = {0};

  //  vkglTF::Model& model = skybox;

  //  vkCmdBindVertexBuffers(currentCommandBuffer, 0, 1, &model.vertices.buffer,
  //                         offsets);
  //  if (model.indices.buffer != VK_NULL_HANDLE) {
  //    vkCmdBindIndexBuffer(currentCommandBuffer, model.indices.buffer, 0,
  //                         VK_INDEX_TYPE_UINT32);
  //  }

  //  vkCmdBindPipeline(currentCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
  //                    pipelines.shadow);
  //  vkCmdBindDescriptorSets(
  //      currentCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
  //      pipelineLayouts.shadow, 0, 1,
  //      &dynamicDescriptorSets[currentFrameIndex].shadow, 0, nullptr);

  //  // Opaque primitives first
  //  for (auto node : model.nodes) {
  //    renderNode(node, currentFrameIndex, vkglTF::Material::ALPHAMODE_OPAQUE,
  //               currentCommandBuffer, true);
  //  }
  //  // Alpha masked primitives
  //  for (auto node : model.nodes) {
  //    renderNode(node, currentFrameIndex, vkglTF::Material::ALPHAMODE_MASK,
  //               currentCommandBuffer, true);
  //  }

  //  VK_CHECK_RESULT(vkEndCommandBuffer(currentCommandBuffer));
  //}

  void renderNode(vkglTF::Node* node, uint32_t cbIndex,
                  vkglTF::Material::AlphaMode alphaMode, VkCommandBuffer curBuf,
                  PushConstData pushConst, bool isShadow = false) {
    if (node->mesh) {
      // Render mesh primitives
      for (vkglTF::Primitive* primitive : node->mesh->primitives) {
        if (isShadow) {
          if (primitive->hasIndices) {
            vkCmdDrawIndexed(curBuf, primitive->indexCount, 1,
                             primitive->firstIndex, 0, 0);
          } else {
            vkCmdDraw(curBuf, primitive->vertexCount, 1, 0, 0);
          }
        } else if (primitive->material.alphaMode == alphaMode) {
          std::string pipelineName = "pbr";
          std::string pipelineVariant = "";

          if (primitive->material.unlit) {
            // KHR_materials_unlit
            pipelineName = "unlit";
          };

          // Material properties define if we e.g. need to bind a pipeline
          // variant with culling disabled (double sided)
          if (alphaMode == vkglTF::Material::ALPHAMODE_BLEND) {
            pipelineVariant = "_alpha_blending";
          } else {
            if (primitive->material.doubleSided) {
              pipelineVariant = "_double_sided";
            }
          }

          const VkPipeline pipeline =
              genPipelines[pipelineName + pipelineVariant];

          if (pipeline != boundPipeline) {
            vkCmdBindPipeline(curBuf, VK_PIPELINE_BIND_POINT_GRAPHICS,
                              pipeline);
            boundPipeline = pipeline;
          }

          const std::vector<VkDescriptorSet> descriptorsets = {
              dynamicDescriptorSets[currentFrameIndex].scene,
              primitive->material.descriptorSet,
              node->mesh->uniformBuffer.descriptorSet,
              staticDescriptorSets.materials};
          vkCmdBindDescriptorSets(curBuf, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                  pipelineLayouts.scene, 0,
                                  static_cast<uint32_t>(descriptorsets.size()),
                                  descriptorsets.data(), 0, NULL);

          pushConst.materialIndex = primitive->material.index;
          vkCmdPushConstants(
              curBuf, pipelineLayouts.scene,
              VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
              sizeof(pushConst), &pushConst);

          if (primitive->hasIndices) {
            vkCmdDrawIndexed(curBuf, primitive->indexCount, 1,
                             primitive->firstIndex, 0, 0);
          } else {
            vkCmdDraw(curBuf, primitive->vertexCount, 1, 0, 0);
          }
        }
      }
    }

    for (auto child : node->children) {
      renderNode(child, cbIndex, alphaMode, curBuf, pushConst, isShadow);
    }
  }

  void getObjectsToRender() {
    dynamicModelsToRenderIndices.clear();
    for (uint32_t i = 0; i < dynamicModels.size(); i++) {
      dynamicModelsToRenderIndices.push_back(i);
    }
  }

  void buildCommandBuffer() override {
    getObjectsToRender();
    // Contains the list of secondary command buffers to be submitted
    std::vector<VkCommandBuffer> secondaryCmdBufs;

    BaseRenderer::buildCommandBuffer();
    VkCommandBufferBeginInfo cmdBufInfo =
        vks::initializers::commandBufferBeginInfo();

    VkClearValue clearValues[2];
    clearValues[0].color = {{uiSettings.skyboxColor.x, uiSettings.skyboxColor.y,
                             uiSettings.skyboxColor.z,
                             uiSettings.skyboxColor.w}};
    clearValues[1].depthStencil = {1.0f, 0};

    VkRenderPassBeginInfo renderPassBeginInfo =
        vks::initializers::renderPassBeginInfo();

    renderPassBeginInfo.renderArea.offset.x = 0;
    renderPassBeginInfo.renderArea.offset.y = 0;
    renderPassBeginInfo.renderArea.extent.width = shadowMapSize;
    renderPassBeginInfo.renderArea.extent.height = shadowMapSize;
    renderPassBeginInfo.clearValueCount = 1;
    renderPassBeginInfo.pClearValues = &clearValues[1];

    newUIFrame((frameCounter == 0));
    imGui->updateBuffers(currentFrameIndex);
    updateGenericUBO();
    updateSceneParams();

    VkCommandBuffer currentCommandBuffer = drawCmdBuffers[currentFrameIndex];

    VK_CHECK_RESULT(vkBeginCommandBuffer(currentCommandBuffer, &cmdBufInfo));

    // Shadows Rendering
    {
      /* renderPassBeginInfo.renderPass = offscreenShadowPass.renderPass;
       renderPassBeginInfo.framebuffer =
           offscreenShadowPass.framebuffers[currentFrameIndex].framebuffer;

       vkCmdBeginRenderPass(currentCommandBuffer, &renderPassBeginInfo,
                            VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

       buildShadowCommandBuffer();

       secondaryCmdBufs.push_back(commandBuffers.shadow[currentFrameIndex]);

       vkCmdExecuteCommands(currentCommandBuffer,
                            static_cast<uint32_t>(secondaryCmdBufs.size()),
                            secondaryCmdBufs.data());
       vkCmdEndRenderPass(currentCommandBuffer);
       secondaryCmdBufs.clear();*/
    }

    renderPassBeginInfo.renderArea.extent.width = getWidth();
    renderPassBeginInfo.renderArea.extent.height = getHeight();
    renderPassBeginInfo.clearValueCount = 2;
    renderPassBeginInfo.pClearValues = clearValues;

    // Scene Rendering
    {
      renderPassBeginInfo.renderPass = postPasses[0]->renderPass;
      renderPassBeginInfo.framebuffer =
          postPasses[0]->framebuffers[currentFrameIndex].framebuffer;

      vkCmdBeginRenderPass(currentCommandBuffer, &renderPassBeginInfo,
                           VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

      buildSceneCommandBuffer();

      secondaryCmdBufs.push_back(commandBuffers.scene[currentFrameIndex]);

      // Execute render commands from the secondary command buffer
      vkCmdExecuteCommands(currentCommandBuffer,
                           static_cast<uint32_t>(secondaryCmdBufs.size()),
                           secondaryCmdBufs.data());
      secondaryCmdBufs.clear();
      vkCmdEndRenderPass(currentCommandBuffer);
    }

    // Post Processing & UI Rendering
    {
      // O -> O+1 if O+1 < postPasses.Size() --- Foreach Post Processing Pass,
      // render into the next
      // Then, Render final image into Swapchain image
      for (size_t i = 0; i < postPasses.size() - 1; i++) {
        renderPassBeginInfo.renderPass = postPasses[i + 1]->renderPass;
        renderPassBeginInfo.framebuffer =
            postPasses[i + 1]->framebuffers[currentFrameIndex].framebuffer;

        vkCmdBeginRenderPass(currentCommandBuffer, &renderPassBeginInfo,
                             VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

        VkCommandBufferInheritanceInfo inheritanceInfo =
            vks::initializers::commandBufferInheritanceInfo();
        inheritanceInfo.renderPass = postPasses[i + 1]->renderPass;
        inheritanceInfo.framebuffer =
            postPasses[i + 1]->framebuffers[currentFrameIndex].framebuffer;

        VkCommandBufferBeginInfo cmdBufInfo =
            vks::initializers::commandBufferBeginInfo();
        cmdBufInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
        cmdBufInfo.pInheritanceInfo = &inheritanceInfo;

        VkCommandBuffer curBuf = postPasses[i]->cmdBufs[currentFrameIndex];
        vkResetCommandBuffer(curBuf, 0);
        VK_CHECK_RESULT(vkBeginCommandBuffer(curBuf, &cmdBufInfo));

        VkViewport viewport = vks::initializers::viewport(
            (float)getWidth(), (float)getHeight(), 0.0f, 1.0f);
        vkCmdSetViewport(curBuf, 0, 1, &viewport);
        VkRect2D scissor =
            vks::initializers::rect2D(getWidth(), getHeight(), 0, 0);
        vkCmdSetScissor(curBuf, 0, 1, &scissor);

        VkDeviceSize offsets[1] = {0};

        vkCmdBindDescriptorSets(
            curBuf, VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipelineLayouts.postProcessing, 0, 1,
            &postPasses[i]->descriptorSets[currentFrameIndex], 0, NULL);
        vkCmdBindPipeline(curBuf, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          postPasses[i]->pipeline);

        // Draw triangle
        vkCmdDraw(curBuf, 3, 1, 0, 0);

        VK_CHECK_RESULT(vkEndCommandBuffer(curBuf));

        secondaryCmdBufs.push_back(curBuf);

        // Execute render commands from the secondary command buffer
        vkCmdExecuteCommands(currentCommandBuffer,
                             static_cast<uint32_t>(secondaryCmdBufs.size()),
                             secondaryCmdBufs.data());
        secondaryCmdBufs.clear();
        vkCmdEndRenderPass(currentCommandBuffer);
      }

      renderPassBeginInfo.renderPass = renderPass;
      renderPassBeginInfo.framebuffer = frameBuffers[currentFrameIndex];

      vkCmdBeginRenderPass(currentCommandBuffer, &renderPassBeginInfo,
                           VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

      buildPostProcessingCommandBuffer();
      buildUICommandBuffer();

      secondaryCmdBufs.push_back(
          commandBuffers.postProcessing[currentFrameIndex]);
      secondaryCmdBufs.push_back(commandBuffers.ui[currentFrameIndex]);

      // Execute render commands from the secondary command buffer
      vkCmdExecuteCommands(currentCommandBuffer,
                           static_cast<uint32_t>(secondaryCmdBufs.size()),
                           secondaryCmdBufs.data());

      vkCmdEndRenderPass(currentCommandBuffer);
    }

    VK_CHECK_RESULT(vkEndCommandBuffer(currentCommandBuffer));
  }

  void windowResized() override {
    BaseRenderer::windowResized();

    for (auto& pass : postPasses) {
      pass->onResize(swapChain.imageCount, getWidth(), getHeight());
    }

    setupDescriptors();
  }

  void setupDescriptors(bool updatedMaterials = false) {
    /*
            Descriptor Pool
    */

    if (descriptorPool == VK_NULL_HANDLE) {
      uint32_t imageSamplerCount = 0;
      uint32_t materialCount = 0;
      uint32_t meshCount = 0;

      // Environment samplers (radiance, irradiance, brdf lut)
      imageSamplerCount += 3;
      // Screen Texture
      imageSamplerCount += 1;
      // Shadows?
      meshCount += 6;
      dynamicDescriptorSets.resize(swapChain.imageCount);
      for (auto& model : dynamicModels) {
        for (auto& material : model.materials) {
          imageSamplerCount += 5;
          materialCount++;
        }
        for (auto node : model.linearNodes) {
          if (node->mesh) {
            meshCount++;
          }
        }
      }

      std::vector<VkDescriptorPoolSize> poolSizes = {
          {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
           (4 + meshCount) * swapChain.imageCount},
          {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
           imageSamplerCount * swapChain.imageCount},
          // One SSBO for the shader material buffer
          {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1}};
      VkDescriptorPoolCreateInfo descriptorPoolCI{};
      descriptorPoolCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
      descriptorPoolCI.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
      descriptorPoolCI.pPoolSizes = poolSizes.data();
      descriptorPoolCI.maxSets =
          (2 + materialCount + meshCount) * swapChain.imageCount;
      VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolCI, nullptr,
                                             &descriptorPool));
    }
    /*
            Descriptor sets
    */

    // Scene (matrices and environment maps)
    {
      if (descriptorSetLayouts.scene == VK_NULL_HANDLE) {
        std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
            {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1,
             VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
             nullptr},
            {1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1,
             VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
            {2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
             VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
            {3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
             VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
            {4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
             VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
            {5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
             VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
        };
        VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCI{};
        descriptorSetLayoutCI.sType =
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptorSetLayoutCI.pBindings = setLayoutBindings.data();
        descriptorSetLayoutCI.bindingCount =
            static_cast<uint32_t>(setLayoutBindings.size());
        VK_CHECK_RESULT(
            vkCreateDescriptorSetLayout(device, &descriptorSetLayoutCI, nullptr,
                                        &descriptorSetLayouts.scene));
        for (auto i = 0; i < dynamicDescriptorSets.size(); i++) {
          VkDescriptorSetAllocateInfo descriptorSetAllocInfo{};
          descriptorSetAllocInfo.sType =
              VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
          descriptorSetAllocInfo.descriptorPool = descriptorPool;
          descriptorSetAllocInfo.pSetLayouts = &descriptorSetLayouts.scene;
          descriptorSetAllocInfo.descriptorSetCount = 1;
          VK_CHECK_RESULT(
              vkAllocateDescriptorSets(device, &descriptorSetAllocInfo,
                                       &dynamicDescriptorSets[i].scene));
        }
      }
      for (auto i = 0; i < dynamicDescriptorSets.size(); i++) {
        std::array<VkWriteDescriptorSet, 6> writeDescriptorSets{};

        writeDescriptorSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSets[0].descriptorType =
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writeDescriptorSets[0].descriptorCount = 1;
        writeDescriptorSets[0].dstSet = dynamicDescriptorSets[i].scene;
        writeDescriptorSets[0].dstBinding = 0;
        writeDescriptorSets[0].pBufferInfo =
            &dynamicUniformBuffers[i].scene.descriptor;

        writeDescriptorSets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSets[1].descriptorType =
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writeDescriptorSets[1].descriptorCount = 1;
        writeDescriptorSets[1].dstSet = dynamicDescriptorSets[i].scene;
        writeDescriptorSets[1].dstBinding = 1;
        writeDescriptorSets[1].pBufferInfo =
            &dynamicUniformBuffers[i].params.descriptor;

        writeDescriptorSets[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSets[2].descriptorType =
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writeDescriptorSets[2].descriptorCount = 1;
        writeDescriptorSets[2].dstSet = dynamicDescriptorSets[i].scene;
        writeDescriptorSets[2].dstBinding = 2;
        writeDescriptorSets[2].pImageInfo = &textures.irradianceCube.descriptor;

        writeDescriptorSets[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSets[3].descriptorType =
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writeDescriptorSets[3].descriptorCount = 1;
        writeDescriptorSets[3].dstSet = dynamicDescriptorSets[i].scene;
        writeDescriptorSets[3].dstBinding = 3;
        writeDescriptorSets[3].pImageInfo =
            &textures.prefilteredCube.descriptor;

        writeDescriptorSets[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSets[4].descriptorType =
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writeDescriptorSets[4].descriptorCount = 1;
        writeDescriptorSets[4].dstSet = dynamicDescriptorSets[i].scene;
        writeDescriptorSets[4].dstBinding = 4;
        writeDescriptorSets[4].pImageInfo = &textures.lutBrdf.descriptor;

        writeDescriptorSets[5].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSets[5].descriptorType =
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writeDescriptorSets[5].descriptorCount = 1;
        writeDescriptorSets[5].dstSet = dynamicDescriptorSets[i].scene;
        writeDescriptorSets[5].dstBinding = 5;
        writeDescriptorSets[5].pImageInfo =
            &offscreenShadowPass.framebuffers[i].descriptor;

        vkUpdateDescriptorSets(
            device, static_cast<uint32_t>(writeDescriptorSets.size()),
            writeDescriptorSets.data(), 0, NULL);
      }
    }

    // Material (samplers)
    {
      if (descriptorSetLayouts.material == VK_NULL_HANDLE || updatedMaterials) {
        if (updatedMaterials) {
          vkDestroyDescriptorSetLayout(device, descriptorSetLayouts.material,
                                       nullptr);
        }

        std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
            {0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
             VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
            {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
             VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
            {2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
             VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
            {3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
             VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
            {4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
             VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
        };
        VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCI{};
        descriptorSetLayoutCI.sType =
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptorSetLayoutCI.pBindings = setLayoutBindings.data();
        descriptorSetLayoutCI.bindingCount =
            static_cast<uint32_t>(setLayoutBindings.size());
        VK_CHECK_RESULT(
            vkCreateDescriptorSetLayout(device, &descriptorSetLayoutCI, nullptr,
                                        &descriptorSetLayouts.material));
        for (auto& model : dynamicModels) {
          for (auto& material : model.materials) {
            VkDescriptorSetAllocateInfo descriptorSetAllocInfo{};
            descriptorSetAllocInfo.sType =
                VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            descriptorSetAllocInfo.descriptorPool = descriptorPool;
            descriptorSetAllocInfo.pSetLayouts = &descriptorSetLayouts.material;
            descriptorSetAllocInfo.descriptorSetCount = 1;
            VK_CHECK_RESULT(vkAllocateDescriptorSets(
                device, &descriptorSetAllocInfo, &material.descriptorSet));
          }
        }
      }
      // Per-Material descriptor sets
      for (auto& model : dynamicModels) {
        for (auto& material : model.materials) {
          std::vector<VkDescriptorImageInfo> imageDescriptors = {
              textures.empty.descriptor, textures.empty.descriptor,
              material.normalTexture ? material.normalTexture->descriptor
                                     : textures.empty.descriptor,
              material.occlusionTexture ? material.occlusionTexture->descriptor
                                        : textures.empty.descriptor,
              material.emissiveTexture ? material.emissiveTexture->descriptor
                                       : textures.empty.descriptor};

          if (material.pbrWorkflows.metallicRoughness) {
            if (material.baseColorTexture) {
              imageDescriptors[0] = material.baseColorTexture->descriptor;
            }
            if (material.metallicRoughnessTexture) {
              imageDescriptors[1] =
                  material.metallicRoughnessTexture->descriptor;
            }
          } else {
            if (material.pbrWorkflows.specularGlossiness) {
              if (material.extension.diffuseTexture) {
                imageDescriptors[0] =
                    material.extension.diffuseTexture->descriptor;
              }
              if (material.extension.specularGlossinessTexture) {
                imageDescriptors[1] =
                    material.extension.specularGlossinessTexture->descriptor;
              }
            }
          }

          std::array<VkWriteDescriptorSet, 5> writeDescriptorSets{};
          for (size_t i = 0; i < imageDescriptors.size(); i++) {
            writeDescriptorSets[i].sType =
                VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writeDescriptorSets[i].descriptorType =
                VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            writeDescriptorSets[i].descriptorCount = 1;
            writeDescriptorSets[i].dstSet = material.descriptorSet;
            writeDescriptorSets[i].dstBinding = static_cast<uint32_t>(i);
            writeDescriptorSets[i].pImageInfo = &imageDescriptors[i];
          }

          vkUpdateDescriptorSets(
              device, static_cast<uint32_t>(writeDescriptorSets.size()),
              writeDescriptorSets.data(), 0, NULL);
        }
      }

      // Model node (matrices)
      {
        if (descriptorSetLayouts.node == VK_NULL_HANDLE || updatedMaterials) {
          if (updatedMaterials) {
            vkDestroyDescriptorSetLayout(device, descriptorSetLayouts.node,
                                         nullptr);
          }
          std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
              {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1,
               VK_SHADER_STAGE_VERTEX_BIT, nullptr},
          };
          VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCI{};
          descriptorSetLayoutCI.sType =
              VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
          descriptorSetLayoutCI.pBindings = setLayoutBindings.data();
          descriptorSetLayoutCI.bindingCount =
              static_cast<uint32_t>(setLayoutBindings.size());
          VK_CHECK_RESULT(
              vkCreateDescriptorSetLayout(device, &descriptorSetLayoutCI,
                                          nullptr, &descriptorSetLayouts.node));
          // Per-Node descriptor set

          for (auto& model : dynamicModels) {
            for (auto& node : model.nodes) {
              allocateNodeDescriptorSet(node);
            }
          }
        }
        // Per-Node descriptor set
        for (auto& model : dynamicModels) {
          for (auto& node : model.nodes) {
            setupNodeDescriptorSet(node);
          }
        }
      }

      // Material Buffer
      {
        if (descriptorSetLayouts.materialBuffer == VK_NULL_HANDLE ||
            updatedMaterials) {
          if (updatedMaterials) {
            vkDestroyDescriptorSetLayout(
                device, descriptorSetLayouts.materialBuffer, nullptr);
          }
          std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
              {0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1,
               VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
          };
          VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCI{};
          descriptorSetLayoutCI.sType =
              VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
          descriptorSetLayoutCI.pBindings = setLayoutBindings.data();
          descriptorSetLayoutCI.bindingCount =
              static_cast<uint32_t>(setLayoutBindings.size());
          VK_CHECK_RESULT(vkCreateDescriptorSetLayout(
              device, &descriptorSetLayoutCI, nullptr,
              &descriptorSetLayouts.materialBuffer));

          VkDescriptorSetAllocateInfo descriptorSetAllocInfo{};
          descriptorSetAllocInfo.sType =
              VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
          descriptorSetAllocInfo.descriptorPool = descriptorPool;
          descriptorSetAllocInfo.pSetLayouts =
              &descriptorSetLayouts.materialBuffer;
          descriptorSetAllocInfo.descriptorSetCount = 1;
          VK_CHECK_RESULT(
              vkAllocateDescriptorSets(device, &descriptorSetAllocInfo,
                                       &staticDescriptorSets.materials));
        }

        for (auto& model : dynamicModels) {
          std::array<VkWriteDescriptorSet, 1> writeDescriptorSets{};
          writeDescriptorSets[0] = vks::initializers::writeDescriptorSet(
              staticDescriptorSets.materials, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
              0, &model.materialBuffer.descriptor);
          vkUpdateDescriptorSets(
              device, static_cast<uint32_t>(writeDescriptorSets.size()),
              writeDescriptorSets.data(), 0, nullptr);
        }
      }
    }

    // Skybox (fixed set)
    {
      if (descriptorSetLayouts.skybox == VK_NULL_HANDLE) {
        std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
            {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1,
             VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
             nullptr},
            {1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1,
             VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
            {2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
             VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
        };
        VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCI{};
        descriptorSetLayoutCI.sType =
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptorSetLayoutCI.pBindings = setLayoutBindings.data();
        descriptorSetLayoutCI.bindingCount =
            static_cast<uint32_t>(setLayoutBindings.size());
        VK_CHECK_RESULT(
            vkCreateDescriptorSetLayout(device, &descriptorSetLayoutCI, nullptr,
                                        &descriptorSetLayouts.skybox));
        for (auto i = 0; i < dynamicUniformBuffers.size(); i++) {
          VkDescriptorSetAllocateInfo descriptorSetAllocInfo{};
          descriptorSetAllocInfo.sType =
              VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
          descriptorSetAllocInfo.descriptorPool = descriptorPool;
          descriptorSetAllocInfo.pSetLayouts = &descriptorSetLayouts.skybox;
          descriptorSetAllocInfo.descriptorSetCount = 1;
          VK_CHECK_RESULT(
              vkAllocateDescriptorSets(device, &descriptorSetAllocInfo,
                                       &dynamicDescriptorSets[i].skybox));
        }
      }

      for (auto i = 0; i < dynamicUniformBuffers.size(); i++) {
        std::array<VkWriteDescriptorSet, 3> writeDescriptorSets{};

        writeDescriptorSets[0] = vks::initializers::writeDescriptorSet(
            dynamicDescriptorSets[i].skybox, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            0, &dynamicUniformBuffers[i].scene.descriptor);

        writeDescriptorSets[1] = vks::initializers::writeDescriptorSet(
            dynamicDescriptorSets[i].skybox, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            1, &dynamicUniformBuffers[i].params.descriptor);

        writeDescriptorSets[2] = vks::initializers::writeDescriptorSet(
            dynamicDescriptorSets[i].skybox,
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2,
            &textures.environmentCube.descriptor);

        vkUpdateDescriptorSets(
            device, static_cast<uint32_t>(writeDescriptorSets.size()),
            writeDescriptorSets.data(), 0, nullptr);
      }
    }

    // Post Processing
    {
      if (postPasses[0]->descriptorSets.size() == 0) {
        std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
            {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1,
             VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
            {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
             VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
        };
        VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCI{};
        descriptorSetLayoutCI.sType =
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptorSetLayoutCI.pBindings = setLayoutBindings.data();
        descriptorSetLayoutCI.bindingCount =
            static_cast<uint32_t>(setLayoutBindings.size());
        VK_CHECK_RESULT(
            vkCreateDescriptorSetLayout(device, &descriptorSetLayoutCI, nullptr,
                                        &descriptorSetLayouts.postProcessing));

        VkDescriptorSetAllocateInfo descriptorSetAllocInfo{};
        VkDescriptorSetAllocateInfo allocInfo =
            vks::initializers::descriptorSetAllocateInfo(
                descriptorPool, &descriptorSetLayouts.postProcessing, 1);

        for (auto& pass : postPasses) {
          pass->descriptorSets.resize(dynamicDescriptorSets.size());
          for (auto i = 0; i < dynamicDescriptorSets.size(); i++) {
            descriptorSetAllocInfo.sType =
                VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            descriptorSetAllocInfo.descriptorPool = descriptorPool;
            descriptorSetAllocInfo.pSetLayouts =
                &descriptorSetLayouts.postProcessing;
            descriptorSetAllocInfo.descriptorSetCount = 1;
            VK_CHECK_RESULT(vkAllocateDescriptorSets(
                device, &descriptorSetAllocInfo, &pass->descriptorSets[i]));
          }
        }
      }

      // Post Processing (fixed set)
      for (auto& pass : postPasses) {
        for (size_t i = 0; i < swapChain.imageCount; i++) {
          std::array<VkWriteDescriptorSet, 2> writeDescriptorSets{};

          writeDescriptorSets[0] = vks::initializers::writeDescriptorSet(
              pass->descriptorSets[i], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0,
              &staticUniformBuffers.postProcessing.descriptor);

          writeDescriptorSets[1] = vks::initializers::writeDescriptorSet(
              pass->descriptorSets[i],
              VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
              &pass->framebuffers[i].descriptor);

          vkUpdateDescriptorSets(
              device, static_cast<uint32_t>(writeDescriptorSets.size()),
              writeDescriptorSets.data(), 0, nullptr);
        }
      }
    }

    // Shadows
    {
      if (descriptorSetLayouts.shadow == VK_NULL_HANDLE) {
        std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
            vks::initializers::descriptorSetLayoutBinding(
                VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT,
                0),
        };
        VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCI{};
        descriptorSetLayoutCI.sType =
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptorSetLayoutCI.pBindings = setLayoutBindings.data();
        descriptorSetLayoutCI.bindingCount =
            static_cast<uint32_t>(setLayoutBindings.size());
        VK_CHECK_RESULT(
            vkCreateDescriptorSetLayout(device, &descriptorSetLayoutCI, nullptr,
                                        &descriptorSetLayouts.shadow));
        for (auto i = 0; i < dynamicUniformBuffers.size(); i++) {
          VkDescriptorSetAllocateInfo descriptorSetAllocInfo{};
          descriptorSetAllocInfo.sType =
              VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
          descriptorSetAllocInfo.descriptorPool = descriptorPool;
          descriptorSetAllocInfo.pSetLayouts = &descriptorSetLayouts.shadow;
          descriptorSetAllocInfo.descriptorSetCount = 1;
          VK_CHECK_RESULT(
              vkAllocateDescriptorSets(device, &descriptorSetAllocInfo,
                                       &dynamicDescriptorSets[i].shadow));
        }
      }
      for (auto i = 0; i < dynamicUniformBuffers.size(); i++) {
        std::array<VkWriteDescriptorSet, 1> writeDescriptorSets = {
            // Binding 0 : Vertex shader uniform buffer
            vks::initializers::writeDescriptorSet(
                dynamicDescriptorSets[i].shadow,
                VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0,
                &dynamicUniformBuffers[i].shadow.descriptor),
        };
        vkUpdateDescriptorSets(
            device, static_cast<uint32_t>(writeDescriptorSets.size()),
            writeDescriptorSets.data(), 0, nullptr);
      }
    }
  }

  void addPipelineSet(const std::string prefix, const std::string vertexShader,
                      const std::string fragmentShader,
                      bool emptyVertexInput = false) {
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCI =
        vks::initializers::pipelineInputAssemblyStateCreateInfo(
            VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);

    VkPipelineRasterizationStateCreateInfo rasterizationStateCI =
        vks::initializers::pipelineRasterizationStateCreateInfo(
            VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT,
            VK_FRONT_FACE_COUNTER_CLOCKWISE);

    VkPipelineColorBlendAttachmentState blendAttachmentState =
        vks::initializers::pipelineColorBlendAttachmentState(
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
            VK_FALSE);

    VkPipelineColorBlendStateCreateInfo colorBlendStateCI =
        vks::initializers::pipelineColorBlendStateCreateInfo(
            1, &blendAttachmentState);

    VkPipelineDepthStencilStateCreateInfo depthStencilStateCI =
        vks::initializers::pipelineDepthStencilStateCreateInfo(
            VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);

    VkPipelineViewportStateCreateInfo viewportStateCI =
        vks::initializers::pipelineViewportStateCreateInfo(1, 1);

    VkPipelineMultisampleStateCreateInfo multisampleStateCI =
        vks::initializers::pipelineMultisampleStateCreateInfo(
            getMSAASampleCount());

    std::vector<VkDynamicState> dynamicStateEnables = {
        VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicStateCI =
        vks::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables);

    // Pipeline layout
    const std::vector<VkDescriptorSetLayout> setLayouts = {
        descriptorSetLayouts.scene, descriptorSetLayouts.material,
        descriptorSetLayouts.node, descriptorSetLayouts.materialBuffer};
    VkPipelineLayoutCreateInfo pipelineLayoutCI{};
    pipelineLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCI.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
    pipelineLayoutCI.pSetLayouts = setLayouts.data();
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.size = sizeof(PushConstData);
    pushConstantRange.stageFlags =
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pipelineLayoutCI.pushConstantRangeCount = 1;
    pipelineLayoutCI.pPushConstantRanges = &pushConstantRange;
    VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCI, nullptr,
                                           &pipelineLayouts.scene));

    // Vertex bindings and attributes
    VkVertexInputBindingDescription vertexInputBinding = {
        0, sizeof(vkglTF::Vertex), VK_VERTEX_INPUT_RATE_VERTEX};
    std::vector<VkVertexInputAttributeDescription> vertexInputAttributes = {
        {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(vkglTF::Vertex, pos)},
        {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(vkglTF::Vertex, normal)},
        {2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(vkglTF::Vertex, uv0)},
        {3, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(vkglTF::Vertex, uv1)},
        {4, 0, VK_FORMAT_R32G32B32A32_UINT, offsetof(vkglTF::Vertex, joint0)},
        {5, 0, VK_FORMAT_R32G32B32A32_SFLOAT,
         offsetof(vkglTF::Vertex, weight0)},
        {6, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(vkglTF::Vertex, color)}};

    VkPipelineVertexInputStateCreateInfo vertexInputStateCI{};
    vertexInputStateCI.sType =
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputStateCI.vertexBindingDescriptionCount = 1;
    vertexInputStateCI.pVertexBindingDescriptions = &vertexInputBinding;
    vertexInputStateCI.vertexAttributeDescriptionCount =
        static_cast<uint32_t>(vertexInputAttributes.size());
    vertexInputStateCI.pVertexAttributeDescriptions =
        vertexInputAttributes.data();

    // Pipelines
    std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;

    VkGraphicsPipelineCreateInfo pipelineCI{};
    pipelineCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineCI.layout = pipelineLayouts.scene;
    pipelineCI.renderPass = postPasses[0]->renderPass;
    pipelineCI.pInputAssemblyState = &inputAssemblyStateCI;
    pipelineCI.pVertexInputState = &vertexInputStateCI;
    pipelineCI.pRasterizationState = &rasterizationStateCI;
    pipelineCI.pColorBlendState = &colorBlendStateCI;
    pipelineCI.pMultisampleState = &multisampleStateCI;
    pipelineCI.pViewportState = &viewportStateCI;
    pipelineCI.pDepthStencilState = &depthStencilStateCI;
    pipelineCI.pDynamicState = &dynamicStateCI;
    pipelineCI.stageCount = static_cast<uint32_t>(shaderStages.size());
    pipelineCI.pStages = shaderStages.data();

    shaderStages[0] = loadShader(vertexShader, VK_SHADER_STAGE_VERTEX_BIT);
    shaderStages[1] = loadShader(fragmentShader, VK_SHADER_STAGE_FRAGMENT_BIT);

    VkPipeline pipeline{};
    // Default pipeline with back-face culling
    VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1,
                                              &pipelineCI, nullptr, &pipeline));
    genPipelines[prefix] = pipeline;
    // Double sided
    rasterizationStateCI.cullMode = VK_CULL_MODE_NONE;
    VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1,
                                              &pipelineCI, nullptr, &pipeline));
    genPipelines[prefix + "_double_sided"] = pipeline;
    // Alpha blending
    rasterizationStateCI.cullMode = VK_CULL_MODE_NONE;
    blendAttachmentState.blendEnable = VK_TRUE;
    blendAttachmentState.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    blendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    blendAttachmentState.dstColorBlendFactor =
        VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
    blendAttachmentState.srcAlphaBlendFactor =
        VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    blendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
    VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1,
                                              &pipelineCI, nullptr, &pipeline));
    genPipelines[prefix + "_alpha_blending"] = pipeline;
  }

  void preparePipelines() {
    // Skybox
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
        vks::initializers::pipelineLayoutCreateInfo(
            &descriptorSetLayouts.skybox, 1);
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.size = sizeof(PushConstData);
    pushConstantRange.stageFlags =
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
    pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;
    VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo,
                                           nullptr, &pipelineLayouts.skybox));
    VkGraphicsPipelineCreateInfo pipelineCI =
        vks::initializers::graphicsPipelineCreateInfo(
            pipelineLayouts.skybox, postPasses[0]->renderPass);

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
         vkglTF::VertexComponent::UV0, vkglTF::VertexComponent::Tangent});

    // Skybox pipeline (background cube)
    rasterizationState.cullMode = VK_CULL_MODE_FRONT_BIT;
    shaderStages[0] =
        loadShader("shaders/skybox.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
    shaderStages[1] =
        loadShader("shaders/skybox.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
    VK_CHECK_RESULT(vkCreateGraphicsPipelines(
        device, pipelineCache, 1, &pipelineCI, nullptr, &pipelines.skybox));

    // Post Processing
    rasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;
    pipelineLayoutCreateInfo = vks::initializers::pipelineLayoutCreateInfo(
        &descriptorSetLayouts.postProcessing, 1);
    VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo,
                                           nullptr,
                                           &pipelineLayouts.postProcessing));

    VkPipelineVertexInputStateCreateInfo emptyInputState{};
    emptyInputState.sType =
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    emptyInputState.vertexAttributeDescriptionCount = 0;
    emptyInputState.pVertexAttributeDescriptions = nullptr;
    emptyInputState.vertexBindingDescriptionCount = 0;
    emptyInputState.pVertexBindingDescriptions = nullptr;

    rasterizationState.cullMode = VK_CULL_MODE_FRONT_BIT;

    for (uint32_t i = 0; i < postPasses.size() - 1; i++) {
      VkGraphicsPipelineCreateInfo postProcessingpipelineCI =
          vks::initializers::graphicsPipelineCreateInfo(
              pipelineLayouts.postProcessing, postPasses[i + 1]->renderPass);
      shaderStages[0] = loadShader(postPasses[i]->vertexShaderPath,
                                   VK_SHADER_STAGE_VERTEX_BIT);
      shaderStages[1] = loadShader(postPasses[i]->fragmentShaderPath,
                                   VK_SHADER_STAGE_FRAGMENT_BIT);

      postProcessingpipelineCI.pInputAssemblyState = &inputAssemblyState;
      postProcessingpipelineCI.pRasterizationState = &rasterizationState;
      postProcessingpipelineCI.pColorBlendState = &colorBlendState;
      postProcessingpipelineCI.pMultisampleState = &multisampleState;
      postProcessingpipelineCI.pViewportState = &viewportState;
      postProcessingpipelineCI.pDepthStencilState = &depthStencilState;
      postProcessingpipelineCI.pDynamicState = &dynamicState;
      postProcessingpipelineCI.pVertexInputState = &emptyInputState;
      postProcessingpipelineCI.stageCount =
          static_cast<uint32_t>(shaderStages.size());
      postProcessingpipelineCI.pStages = shaderStages.data();

      VK_CHECK_RESULT(vkCreateGraphicsPipelines(
          device, nullptr, 1, &postProcessingpipelineCI, nullptr,
          &postPasses[i]->pipeline));
    }

    VkGraphicsPipelineCreateInfo postProcessingpipelineCI =
        vks::initializers::graphicsPipelineCreateInfo(
            pipelineLayouts.postProcessing, renderPass);

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

    shaderStages[0] =
        loadShader(postPasses[postPasses.size() - 1]->vertexShaderPath,
                   VK_SHADER_STAGE_VERTEX_BIT);
    shaderStages[1] =
        loadShader(postPasses[postPasses.size() - 1]->fragmentShaderPath,
                   VK_SHADER_STAGE_FRAGMENT_BIT);

    VK_CHECK_RESULT(vkCreateGraphicsPipelines(
        device, nullptr, 1, &postProcessingpipelineCI, nullptr,
        &postPasses[postPasses.size() - 1]->pipeline));

    // Shadow
    rasterizationState.cullMode = VK_CULL_MODE_NONE;
    pipelineLayoutCreateInfo = vks::initializers::pipelineLayoutCreateInfo(
        &descriptorSetLayouts.shadow, 1);
    VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo,
                                           nullptr, &pipelineLayouts.shadow));

    VkGraphicsPipelineCreateInfo shadowPipelineCI =
        vks::initializers::graphicsPipelineCreateInfo(
            pipelineLayouts.shadow, offscreenShadowPass.renderPass);

    shaderStages[0] =
        loadShader("shaders/shadow.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);

    shadowPipelineCI.pInputAssemblyState = &inputAssemblyState;
    shadowPipelineCI.pRasterizationState = &rasterizationState;
    shadowPipelineCI.pColorBlendState = &colorBlendState;
    shadowPipelineCI.pMultisampleState = &multisampleState;
    shadowPipelineCI.pViewportState = &viewportState;
    shadowPipelineCI.pDepthStencilState = &depthStencilState;
    shadowPipelineCI.pDynamicState = &dynamicState;
    shadowPipelineCI.stageCount = 1;
    shadowPipelineCI.pStages = &shaderStages[0];
    shadowPipelineCI.pVertexInputState =
        vkglTF::Vertex::getPipelineVertexInputState(
            {vkglTF::VertexComponent::Position});

    // No blend attachment states (no color attachments used)
    colorBlendState.attachmentCount = 0;
    depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    // Enable depth bias
    rasterizationState.depthBiasEnable = VK_TRUE;
    // Add depth bias to dynamic state, so we can change it at runtime
    dynamicStateEnables.push_back(VK_DYNAMIC_STATE_DEPTH_BIAS);
    dynamicState =
        vks::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables);

    shadowPipelineCI.renderPass = offscreenShadowPass.renderPass;
    VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1,
                                              &shadowPipelineCI, nullptr,
                                              &pipelines.shadow));

    addPipelineSet("pbr", "shaders/pbr.vert.spv", "shaders/pbr.frag.spv");
  }

  // Generate a BRDF integration map used as a look-up-table (Roughness/dotNV)
  // for Irradiance map (0..1 Scale/Bias)
  void generateBRDFLUT() {
    auto tStart = std::chrono::high_resolution_clock::now();

    const VkFormat format = VK_FORMAT_R16G16_SFLOAT;  // R16G16 is supported
                                                      // pretty much everywhere
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
         vkglTF::VertexComponent::UV0});

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

        skybox.draw(cmdBuf);

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
         vkglTF::VertexComponent::UV0});

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

        skybox.draw(cmdBuf);

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

    sceneParams.prefilteredCubeMipLevels = static_cast<float>(numMips);
    auto tEnd = std::chrono::high_resolution_clock::now();
    auto tDiff =
        std::chrono::duration<double, std::milli>(tEnd - tStart).count();
    std::cout << "Generating pre-filtered enivornment cube with " << numMips
              << " mip levels took " << tDiff << " ms" << std::endl;
  }

  void prepareUniformBuffers() {
    dynamicUniformBuffers.resize(swapChain.imageCount);

    for (auto& uniformBuffer : dynamicUniformBuffers) {
      VK_CHECK_RESULT(vulkanDevice->createBuffer(
          VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
              VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
          &uniformBuffer.scene, sizeof(uboMatrices)));
      VK_CHECK_RESULT(vulkanDevice->createBuffer(
          VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
              VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
          &uniformBuffer.params, sizeof(SceneParams)));
      // VK_CHECK_RESULT(vulkanDevice->createBuffer(
      //     VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      //     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
      //         VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      //     &uniformBuffer.postProcessing, sizeof(postProcessingParams)));
      VK_CHECK_RESULT(vulkanDevice->createBuffer(
          VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
              VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
          &uniformBuffer.shadow, sizeof(shadowParams)));

      uniformBuffer.scene.map();
      uniformBuffer.params.map();
      uniformBuffer.shadow.map();
    }

    VK_CHECK_RESULT(vulkanDevice->createBuffer(
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &staticUniformBuffers.postProcessing, sizeof(PostProcessingParams)));
    staticUniformBuffers.postProcessing.map();

    updateGenericUBO();
    updateSceneParams();
    updatePostProcessingParams();
  }

  // All materials for the current scene are stored in an SSBO allowing indexing
  // from a push constant set per primitive
  void createMaterialBuffer() {
    std::vector<ShaderMaterial> shaderMaterials{};
    for (auto& model : dynamicModels) {
      for (auto& material : model.materials) {
        ShaderMaterial shaderMaterial{};

        shaderMaterial.emissiveFactor = material.emissiveFactor;
        // To save space, availabilty and texture coordinate set are combined
        // -1 = texture not used for this material, >= 0 texture used and index
        // of texture coordinate set
        shaderMaterial.colorTextureSet = material.baseColorTexture != nullptr
                                             ? material.texCoordSets.baseColor
                                             : -1;
        shaderMaterial.normalTextureSet = material.normalTexture != nullptr
                                              ? material.texCoordSets.normal
                                              : -1;
        shaderMaterial.occlusionTextureSet =
            material.occlusionTexture != nullptr
                ? material.texCoordSets.occlusion
                : -1;
        shaderMaterial.emissiveTextureSet = material.emissiveTexture != nullptr
                                                ? material.texCoordSets.emissive
                                                : -1;
        shaderMaterial.alphaMask = static_cast<float>(
            material.alphaMode == vkglTF::Material::ALPHAMODE_MASK);
        shaderMaterial.alphaMaskCutoff = material.alphaCutoff;
        shaderMaterial.emissiveStrength = material.emissiveStrength;

        if (material.pbrWorkflows.metallicRoughness) {
          // Metallic roughness workflow
          shaderMaterial.workflow =
              static_cast<float>(PBR_WORKFLOW_METALLIC_ROUGHNESS);
          shaderMaterial.baseColorFactor = material.baseColorFactor;
          shaderMaterial.metallicFactor = material.metallicFactor;
          shaderMaterial.roughnessFactor = material.roughnessFactor;
          shaderMaterial.PhysicalDescriptorTextureSet =
              material.metallicRoughnessTexture != nullptr
                  ? material.texCoordSets.metallicRoughness
                  : -1;
          shaderMaterial.colorTextureSet = material.baseColorTexture != nullptr
                                               ? material.texCoordSets.baseColor
                                               : -1;
        } else {
          if (material.pbrWorkflows.specularGlossiness) {
            // Specular glossiness workflow
            shaderMaterial.workflow =
                static_cast<float>(PBR_WORKFLOW_SPECULAR_GLOSSINESS);
            shaderMaterial.PhysicalDescriptorTextureSet =
                material.extension.specularGlossinessTexture != nullptr
                    ? material.texCoordSets.specularGlossiness
                    : -1;
            shaderMaterial.colorTextureSet =
                material.extension.diffuseTexture != nullptr
                    ? material.texCoordSets.baseColor
                    : -1;
            shaderMaterial.diffuseFactor = material.extension.diffuseFactor;
            shaderMaterial.specularFactor =
                glm::vec4(material.extension.specularFactor, 1.0f);
          }
        }

        shaderMaterials.push_back(shaderMaterial);
      }

      if (model.materialBuffer.buffer != VK_NULL_HANDLE) {
        model.materialBuffer.destroy();
      }
      VkDeviceSize bufferSize = shaderMaterials.size() * sizeof(ShaderMaterial);
      vks::Buffer stagingBuffer;
      VK_CHECK_RESULT(vulkanDevice->createBuffer(
          VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
              VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
          bufferSize, &stagingBuffer.buffer, &stagingBuffer.memory,
          shaderMaterials.data()));
      VK_CHECK_RESULT(vulkanDevice->createBuffer(
          VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, bufferSize,
          &model.materialBuffer.buffer, &model.materialBuffer.memory));

      // Copy from staging buffers
      VkCommandBuffer copyCmd = vulkanDevice->createCommandBuffer(
          VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
      VkBufferCopy copyRegion{};
      copyRegion.size = bufferSize;
      vkCmdCopyBuffer(copyCmd, stagingBuffer.buffer,
                      model.materialBuffer.buffer, 1, &copyRegion);
      vulkanDevice->flushCommandBuffer(copyCmd, graphicsQueue, true);
      stagingBuffer.device = device;
      stagingBuffer.destroy();

      // Update descriptor
      model.materialBuffer.descriptor.buffer = model.materialBuffer.buffer;
      model.materialBuffer.descriptor.offset = 0;
      model.materialBuffer.descriptor.range = bufferSize;
      model.materialBuffer.device = device;
    }
  }

  void updateGenericUBO() {
    // TODO: FOR EACH LIGHT
    //  Matrix from light's point of view
    // glm::mat4 depthProjectionMatrix =
    //     glm::perspective(glm::radians(45.0f), 1.0f, 1.0f, 2.0f);
    // glm::mat4 depthViewMatrix =
    //     glm::lookAt(glm::vec3(iblDir), glm::vec3(0.0f), glm::vec3(0, 1, 0));
    // glm::mat4 depthModelMatrix = glm::mat4(1.0f);
    // TODO THIS IS HAPPENING TWICE, IT SHOULD NOT BE
    // uboMatrices.lightSpace =
    //   (depthProjectionMatrix * depthViewMatrix * depthModelMatrix);

    // shadowParams.depthMVP = uboMatrices.lightSpace;
    // memcpy(dynamicUniformBuffers[currentFrameIndex].shadow.mapped,
    //      &shadowParams, sizeof(shadowParams));

    uboMatrices.projection = camera.matrices.perspective;
    uboMatrices.view = camera.matrices.view;
    glm::mat4 cv = glm::inverse(camera.matrices.view);
    uboMatrices.camPos = glm::vec3(cv[3]);

    skybox.transform.transformMat =
        glm::mat4(glm::mat3(camera.matrices.view)) * skybox.transform.scaleMat;
    uboMatrices.models[0] = skybox.transform.transformMat;
    for (uint32_t i = 0; i < dynamicModelsToRenderIndices.size(); i++) {
      uboMatrices.models[i + 1] =
          dynamicModels[dynamicModelsToRenderIndices[i]].transform.transformMat;
    }
    memcpy(dynamicUniformBuffers[currentFrameIndex].scene.mapped, &uboMatrices,
           sizeof(uboMatrices));
  }

  void updateSceneParams() {
    // IBL
    sceneParams.lights[0].color =
        glm::vec4(1.0f, 1.0f, 1.0f, 10.0f) * uiSettings.IBLstrength;
    sceneParams.lights[0].position = glm::vec4(0.0f, -20.0f, 0.0f, 0.0f);
    sceneParams.lights[0].rotation = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
    sceneParams.lights[0].zNear = 10.0f;
    sceneParams.lights[0].zFar = 60.0f;
    sceneParams.lights[0].lightFOV = 45.0f;

    // Area Light
    sceneParams.lights[1].color = glm::vec4(1.0f, 1.0f, 1.0f, 0.4f);
    if (uiSettings.animateLight) {
      sceneParams.lights[1].position =
          glm::vec4(sin(timer) * 9.0f, 0.0f, -1.0f, 0.0f);
    }
    sceneParams.lights[1].rotation = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
    sceneParams.lights[1].zNear = 10.0f;
    sceneParams.lights[1].zFar = 60.0f;
    sceneParams.lights[1].lightFOV = 45.0f;

    sceneParams.debugViewInputs = uiSettings.debugOutput;
    sceneParams.debugViewLight = uiSettings.debugLight;

    memcpy(dynamicUniformBuffers[currentFrameIndex].params.mapped, &sceneParams,
           sizeof(SceneParams));
  }

  void updatePostProcessingParams() {
    postProcessingParams.aaType = uiSettings.aaMode;
    memcpy(staticUniformBuffers.postProcessing.mapped, &postProcessingParams,
           sizeof(PostProcessingParams));
  }

  void allocateNodeDescriptorSet(vkglTF::Node* node) {
    VkDescriptorSetAllocateInfo descriptorSetAllocInfo{};
    descriptorSetAllocInfo.sType =
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorSetAllocInfo.descriptorPool = descriptorPool;
    descriptorSetAllocInfo.pSetLayouts = &descriptorSetLayouts.node;
    descriptorSetAllocInfo.descriptorSetCount = 1;
    VK_CHECK_RESULT(
        vkAllocateDescriptorSets(device, &descriptorSetAllocInfo,
                                 &node->mesh->uniformBuffer.descriptorSet));
    for (auto& child : node->children) {
      allocateNodeDescriptorSet(child);
    }
  }

  void setupNodeDescriptorSet(vkglTF::Node* node) {
    if (node->mesh) {
      VkWriteDescriptorSet writeDescriptorSet{};
      writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      writeDescriptorSet.descriptorCount = 1;
      writeDescriptorSet.dstSet = node->mesh->uniformBuffer.descriptorSet;
      writeDescriptorSet.dstBinding = 0;
      writeDescriptorSet.pBufferInfo = &node->mesh->uniformBuffer.descriptor;

      vkUpdateDescriptorSets(device, 1, &writeDescriptorSet, 0, nullptr);
    }
    for (auto& child : node->children) {
      setupNodeDescriptorSet(child);
    }
  }

  void loadScene(bool firstTime = false) {
    vkDeviceWaitIdle(device);
    animationIndex = 0;
    animationTimer = 0.0f;

    int index = dynamicModels.size();
    dynamicModels.push_back(vkglTF::Model());
    dynamicModels[index].destroy();
    dynamicModels[index].loadFromFile(
        getAssetPath() + sceneFilePaths[uiSettings.activeSceneIndex],
        vulkanDevice, graphicsQueue);
    dynamicModels[index].transform.updateRotation(
        glm::vec3(0.0f, -90.0f, 0.0f));
    createMaterialBuffer();
    if (!firstTime) {
      setupDescriptors(true);
    }
    // Check and list unsupported extensions
    for (auto& ext : dynamicModels[index].extensions) {
      if (std::find(supportedExtensions.begin(), supportedExtensions.end(),
                    ext) == supportedExtensions.end()) {
        std::cout << "[WARN] Unsupported extension " << ext
                  << " detected. Scene may not work or display as intended\n";
      }
    }
  }

  void loadAssets() {
    std::cout << "Loading assets " << std::endl;
    auto tStart = std::chrono::high_resolution_clock::now();

    const uint32_t glTFLoadingFlags =
        vkglTF::FileLoadingFlags::PreTransformVertices |
        vkglTF::FileLoadingFlags::PreMultiplyVertexColors |
        vkglTF::FileLoadingFlags::FlipY;

    skybox.loadFromFile(getAssetPath() + "models/cube.gltf", vulkanDevice,
                        graphicsQueue, glTFLoadingFlags);
    skybox.transform.updateScale(glm::vec3(10.0f));

    textures.environmentCube.loadFromFile(
        getAssetPath() + "textures/hdr/pisa_cube.ktx",
        VK_FORMAT_R16G16B16A16_SFLOAT, vulkanDevice, graphicsQueue);
    textures.empty.loadFromFile(getAssetPath() + "models/Sponza/white.ktx",
                                VK_FORMAT_R8G8B8A8_UNORM, vulkanDevice,
                                graphicsQueue);
    loadScene(true);

    auto tFileLoad = std::chrono::duration<double, std::milli>(
                         std::chrono::high_resolution_clock::now() - tStart)
                         .count();
    std::cout << "Loading took " << tFileLoad << " ms" << std::endl;
    // loadScene(getAssetPath() +
    //               "glTF-Sample-Models-main/assets/MetalRoughSpheres/glTF/MetalRoughSpheres.gltf",
    //           glTFLoadingFlags);
  }

  void prepareImGui() {
    imGui = new vkImGUI(this);
    imGui->init((float)getWidth(), (float)getHeight());
    imGui->initResources(this, graphicsQueue, getShaderBasePath());
  }

  void preparePostProcessingPasses() {
    postPasses.push_back(new vks::PostProcessingPass(
        vulkanDevice, swapChain.colorFormat, depthFormat, swapChain.imageCount,
        getWidth(), getHeight(), "shaders/postProcessing.vert.spv",
        "shaders/ambientOcclusion.frag.spv"));  // AO
    postPasses.push_back(new vks::PostProcessingPass(
        vulkanDevice, swapChain.colorFormat, depthFormat, swapChain.imageCount,
        getWidth(), getHeight(), "shaders/postProcessing.vert.spv",
        "shaders/antiAliasing.frag.spv"));  // AA
    postPasses.push_back(new vks::PostProcessingPass(
        vulkanDevice, swapChain.colorFormat, depthFormat, swapChain.imageCount,
        getWidth(), getHeight(), "shaders/postProcessing.vert.spv",
        "shaders/tonemapping.frag.spv"));  // TONEMAPPING & COLOR
                                           // CORRECTIONS

    // TODO: THIS IS TEMP
    VkCommandBufferAllocateInfo secondaryGraphicsCmdBufAllocateInfo =
        vks::initializers::commandBufferAllocateInfo(
            graphicsCmdPool, VK_COMMAND_BUFFER_LEVEL_SECONDARY,
            swapChain.imageCount);

    postPasses[0]->cmdBufs.resize(swapChain.imageCount);
    VK_CHECK_RESULT(
        vkAllocateCommandBuffers(device, &secondaryGraphicsCmdBufAllocateInfo,
                                 postPasses[0]->cmdBufs.data()));

    postPasses[1]->cmdBufs.resize(swapChain.imageCount);
    VK_CHECK_RESULT(
        vkAllocateCommandBuffers(device, &secondaryGraphicsCmdBufAllocateInfo,
                                 postPasses[1]->cmdBufs.data()));

    postPasses[2]->cmdBufs.resize(swapChain.imageCount);
    VK_CHECK_RESULT(
        vkAllocateCommandBuffers(device, &secondaryGraphicsCmdBufAllocateInfo,
                                 postPasses[2]->cmdBufs.data()));
  }

  void prepare() override {
    auto tStart = std::chrono::high_resolution_clock::now();
    BaseRenderer::prepare();
    preparePostProcessingPasses();
    loadAssets();
    generateBRDFLUT();
    generateIrradianceCube();
    generatePrefilteredCube();
    prepareUniformBuffers();
    setupDescriptors();
    preparePipelines();
    prepareImGui();
    prepared = true;
    auto tFileLoad = std::chrono::duration<double, std::milli>(
                         std::chrono::high_resolution_clock::now() - tStart)
                         .count();
    std::cout << "Preparing resources took " << tFileLoad << " ms" << std::endl;
  }

  void draw() {
    BaseRenderer::prepareFrame();
    buildCommandBuffer();
    BaseRenderer::submitFrame();
  }

  void render() override {
    if (!prepared) return;

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