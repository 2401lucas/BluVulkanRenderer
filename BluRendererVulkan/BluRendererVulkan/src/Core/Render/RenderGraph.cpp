#include "RenderGraph.hpp"

#include <iostream>

#include "../../libraries/GLTF/tiny_gltf.h"
#include "../Tools/Debug.hpp"
#include "ResourceManagement/ExternalResources/VulkanglTFModel.h"

namespace core_internal::rendering {
RenderGraphPass::RenderGraphPass(RenderGraph* rg, uint32_t index,
                                 const std::string& name)
    : graph(rg), index(index), name(name) {}

void RenderGraphPass::registerShader(
    std::pair<ShaderStagesFlag, std::string> shader) {
  shaders.push_back(shader);
}

void RenderGraphPass::setComputeGroup(glm::vec3 computeGroup) {
  computeGroups = computeGroup;
}

void RenderGraphPass::setSize(glm::vec2 size) { this->size = size; }

void RenderGraphPass::addAttachmentInput(const std::string& name) {
  auto tex = graph->getTexture(name);
  tex->registerPass(index);

  inputTextureAttachments.push_back(tex);
}

RenderTextureResource* RenderGraphPass::addColorOutput(const std::string& name,
                                                       AttachmentInfo info) {
  info.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  info.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;

  auto tex = graph->createTexture(name, info);
  tex->registerPass(index);

  outputColorAttachments.push_back(tex);
  return tex;
}

void RenderGraphPass::setDepthStencilInput(const std::string& name) {
  auto tex = graph->getTexture(name);
  tex->registerPass(index);

  depthStencilInput = tex;
}

void RenderGraphPass::setDepthStencilOutput(const std::string& name,
                                            const AttachmentInfo& info) {
  auto tex = graph->createTexture(name, info);
  tex->registerPass(index);

  depthStencilOutput = tex;
}

RenderBufferResource* RenderGraphPass::addStorageInput(
    const std::string& name) {
  auto buf = graph->getBuffer(name);
  buf->registerPass(index);

  inputStorageAttachments.push_back(buf);
  return buf;
}

RenderBufferResource* RenderGraphPass::addStorageOutput(
    const std::string& name, const BufferInfo& info) {
  auto buf = graph->createBuffer(name, info);
  buf->registerPass(index);

  inputStorageAttachments.push_back(buf);
  return buf;
}

void RenderGraphPass::set_GetCommandBuffer(
    std::function<void(VkCommandBuffer)> callback) {
  recordCommandBuffer_cb = std::move(callback);
}

void RenderGraphPass::setQueue(const RenderGraphQueueFlags& queue) {
  this->queue = queue;
}

RenderGraphQueueFlags& RenderGraphPass::getQueue() { return queue; }

std::vector<RenderTextureResource*>& RenderGraphPass::getOutputAttachments() {
  return outputColorAttachments;
}

std::vector<RenderTextureResource*>& RenderGraphPass::getInputAttachments() {
  return inputTextureAttachments;
}
std::vector<RenderBufferResource*>& RenderGraphPass::getInputStorage() {
  return inputStorageAttachments;
}
std::vector<std::pair<ShaderStagesFlag, std::string>>&
RenderGraphPass::getShaders() {
  return shaders;
}
std::string RenderGraphPass::getName() { return name; }

void RenderGraphPass::draw(VkCommandBuffer buf) {
  if (drawType == core_internal::rendering::RenderGraph::DrawType::Compute) {
    vkCmdBindPipeline(buf, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
    vkCmdBindDescriptorSets(buf, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout,
                            0, 1, &descriptorSet, 0, 0);
    vkCmdDispatch(buf, computeGroups.x, computeGroups.y, computeGroups.z);
    return;
  }

  std::vector<VkRenderingAttachmentInfoKHR> attachments;
  VkRenderingAttachmentInfoKHR depthStencilAttachment;

  if (depthStencilInput) {
    auto tex = graph->getTexture(depthStencilInput->resourceIndex);

    tex->transitionImageLayout(buf,
                               VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);

    depthStencilAttachment = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
        .imageView = tex->view,
        .imageLayout = tex->imageLayout,
        .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
        .storeOp = VK_ATTACHMENT_STORE_OP_NONE,
        .clearValue{.depthStencil = {1.0f, 0}},
    };
  } else if (depthStencilOutput) {
    auto tex = graph->getTexture(depthStencilOutput->resourceIndex);
    tex->transitionImageLayout(
        buf, VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    depthStencilAttachment = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
        .imageView = tex->view,
        .imageLayout = tex->imageLayout,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue{.depthStencil = {1.0f, 0}},
    };
  }

  for (auto& inputTex : inputTextureAttachments) {
    auto tex = graph->getTexture(inputTex->resourceIndex);

    tex->transitionImageLayout(buf, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    VkClearValue clear{};
    clear.color = {0, 0, 0, 0};
    clear.depthStencil = {1.0f, 0};
    attachments.push_back({
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
        .imageView = tex->view,
        .imageLayout = tex->imageLayout,
        .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
        .storeOp = VK_ATTACHMENT_STORE_OP_NONE,
        .clearValue = clear,
    });
  }

  for (auto& outputTex : outputColorAttachments) {
    auto tex = graph->getTexture(outputTex->resourceIndex);
    tex->transitionImageLayout(buf, VK_IMAGE_LAYOUT_UNDEFINED,
                               VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    VkClearValue clear{};
    clear.color = {0, 0, 0, 0};
    clear.depthStencil = {1.0f, 0};

    attachments.push_back({
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
        .imageView = tex->view,
        .imageLayout = tex->imageLayout,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue = clear,
    });
  }

  VkRenderingInfo renderpassInfo{
      .sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR,
      .pNext = nullptr,
      .flags = 0,
      .renderArea = {.extent{.width = size.x, .height = size.y}},
      .layerCount = 1,
      .viewMask = 0,  // Multiview
      .colorAttachmentCount = attachments.size(),
      .pColorAttachments = attachments.data(),
      .pDepthAttachment = &depthStencilAttachment,
      .pStencilAttachment = &depthStencilAttachment,
  };

  vkCmdBeginRenderingKHR(buf, &renderpassInfo);

  vkCmdBindDescriptorSets(buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
                          0, 1, &descriptorSet, 0, nullptr);
  vkCmdBindPipeline(buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

  switch (drawType) {
    case core_internal::rendering::RenderGraph::DrawType::CameraOccludedOpaque:
    case core_internal::rendering::RenderGraph::DrawType::
        CameraOccludedTranslucent:
      vkCmdBindIndexBuffer(buf, graph->getIndexBuffer(), 0,
                           VK_INDEX_TYPE_UINT32);
      vkCmdBindVertexBuffers(buf, 0, 1, graph->getVertexBuffer(), 0);
      vkCmdDrawIndexedIndirect(buf, graph->getDrawBuffer(drawType).buffer, 0, 1,
                               sizeof(float));  // TODO: Fill with non junk
      break;
    case core_internal::rendering::RenderGraph::DrawType::FullscreenTriangle:
      vkCmdDraw(buf, 3, 1, 0, 0);  // Vertices generated in VertexShader
      break;
    case core_internal::rendering::RenderGraph::DrawType::CustomOcclusion:
      break;
    case core_internal::rendering::RenderGraph::DrawType::CPU_RECORDED:
      recordCommandBuffer_cb(buf);
      break;
    default:
      DEBUG_ERROR("Missing Draw Type implementation in pass: " + name);
      break;
  }

  vkCmdEndRenderingKHR(buf);
}

void RenderGraphPass::createDescriptorSetLayout(
    VkDevice device, VkDescriptorSetLayoutCreateInfo* descriptorSetLayoutCI) {
  VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, descriptorSetLayoutCI,
                                              nullptr, &descriptorSetLayout));
}

RenderGraph::BufferHandle RenderGraph::storeBuffer(vulkan::Buffer* buffer) {
  size_t newHandle = buffers.size();
  buffers.push_back(buffer);

  VkWriteDescriptorSet write{
      .dstSet = bindlessDescriptor,
      .dstBinding = buffer->descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
                        ? UniformBinding
                        : StorageBinding,
      .dstArrayElement = newHandle,
      .descriptorCount = 1,
      .pBufferInfo = &buffer->descriptor};

  vkUpdateDescriptorSets(device->logicalDevice, 1, &write, 0, nullptr);
  return static_cast<BufferHandle>(newHandle);
}

RenderGraph::TextureHandle RenderGraph::storeTexture(vulkan::Image* image) {
  size_t newHandle = textures.size();
  textures.push_back(image);

  VkWriteDescriptorSet write{};
  write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  write.dstBinding = TextureBinding;
  write.dstSet = bindlessDescriptor;
  write.descriptorCount = 1;
  write.dstArrayElement = newHandle;
  write.pImageInfo = &image->descriptor;

  vkUpdateDescriptorSets(device->logicalDevice, 1, &write, 0, nullptr);
  return static_cast<TextureHandle>(newHandle);
}

RenderGraphPass* RenderGraph::addPass(const std::string& name,
                                      const DrawType& drawType) {
  RenderGraphPass* newPass =
      new RenderGraphPass(this, renderPasses.size(), name);
  renderPasses.push_back(newPass);
  return newPass;
}

RenderTextureResource* RenderGraph::createTexture(const std::string& name,
                                                  const AttachmentInfo& info) {
  auto newTex = new RenderTextureResource(info);
  auto [it, success] =
      textureBlackboard.try_emplace(name, static_cast<RenderResource*>(newTex));

  if (!success) {
    DEBUG_ERROR("Texture with name: << " + name +
                " >> has already registered to RenderGraph");
  }

  return newTex;
}

RenderBufferResource* RenderGraph::createBuffer(const std::string& name,
                                                const BufferInfo& info) {
  auto newBuf = new RenderBufferResource(info);
  auto [it, success] =
      bufferBlackboard.try_emplace(name, static_cast<RenderResource*>(newBuf));

  if (!success) {
    DEBUG_ERROR("Buffer with name << " + name +
                " >> has already registered to RenderGraph");
  }

  return newBuf;
}

RenderTextureResource* RenderGraph::getTexture(const std::string& name) {
  if (auto res = textureBlackboard.find(name); res != textureBlackboard.end()) {
    return static_cast<RenderTextureResource*>(res->second);
  } else {
    DEBUG_ERROR("Texture with name << " + name +
                " >> has not been registered to RenderGraph, but is being "
                "referenced");
  }
}

vulkan::Image* RenderGraph::getTexture(const uint32_t& index) {
  if (index >= internalRenderImages.size()) {
    DEBUG_ERROR("Trying to get texture that does not exist with index: " +
                index);
  }

  return internalRenderImages[index];
}

RenderBufferResource* RenderGraph::getBuffer(const std::string& name) {
  if (auto res = bufferBlackboard.find(name); res != bufferBlackboard.end()) {
    return static_cast<RenderBufferResource*>(res->second);
  } else {
    DEBUG_ERROR("Buffer << " + name +
                " >> has not been registered to RenderGraph, but is being "
                "referenced");
  }
}

void RenderGraph::validateData() {
  if (renderPasses.size() == 0) {
    DEBUG_ERROR("RenderGraph has no set Render Passes");
  }

  for (auto& rp : renderPasses) {
    auto& outputs = rp->getOutputAttachments();
    uint32_t sizeX = -1, sizeY = -1;
    for (auto& o : outputs) {
      uint32_t width =
          getImageSize(o->renderTextureInfo.sizeRelative, swapchain->imageWidth,
                       o->renderTextureInfo.sizeX);
      uint32_t height =
          getImageSize(o->renderTextureInfo.sizeRelative,
                       swapchain->imageHeight, o->renderTextureInfo.sizeY);
      if (sizeX == -1 && sizeY == -1) {
        sizeX = width;
        sizeY = height;
      } else {
        if (sizeX != width || sizeY != height) {
          DEBUG_ERROR("Not all output attachments sizes match");
        }
      }
    }
    rp->setSize(glm::vec2(sizeX, sizeY));

    auto& shaders = rp->getShaders();
    uint32_t shaderMask = 0;
    for (auto& shader : shaders) {
      shaderMask |= shader.first;
    }
    switch (shaderMask) {
      case SHADER_STAGE_VERTEX:
      case SHADER_STAGE_VERT_FRAG:
      case SHADER_STAGE_COMPUTE:
        break;
      default:
        DEBUG_ERROR("Shader stages not supported in pass: " + rp->getName());
        break;
    }
  }

  if (textureBlackboard.empty() && bufferBlackboard.empty()) {
    DEBUG_ERROR("No outputs defined in RenderGraph's set Render Passes");
  }

  for (auto& tex : textureBlackboard) {
    if (tex.second->usedInPasses.size() < 2) {
      DEBUG_WARNING("Resource with name: " + tex.first +
                    "is only referenced as an output");
    }
  }

  if (finalOutput == nullptr) {
    DEBUG_ERROR("RenderGraph's finalOutput has not been set");
  }
}

void RenderGraph::generateDependencyChain() {
  // Assign Passes DepenencyLayer
  for (uint32_t i = 0; i < renderPasses.size(); i++) {
    // Start the search from every node
    dependencySearch(i, 0);
  }
}

void RenderGraph::dependencySearch(const uint32_t& passIndex,
                                   const uint32_t& depth) {
  // If depth is greater than number of render passes, it is a cyclic loop
  if (renderPasses.size() < depth) {
    DEBUG_ERROR("Fatal: RenderGraph has a cyclic loop");
  }
  // If the dependencyLayer has already been set to a higher depth than the
  // depth we are currently on, we know the same will be true for the children
  // and can early exit this search
  if (renderPasses[passIndex]->dependencyLayer > depth) {
    return;
  }
  renderPasses[passIndex]->dependencyLayer = depth;
  auto& outputResources = renderPasses[passIndex]->getOutputAttachments();

  for (uint32_t j = 0; j < outputResources.size(); j++) {
    outputResources[j]->resourceLifespan |= 1 << depth;

    auto& passes = outputResources[j]->usedInPasses;
    for (uint32_t k = 0; k < passes.size(); k++) {
      dependencySearch(passes[k], depth + 1);
    }
  }
}

void RenderGraph::generateResources() {
  auto& fifCount = swapchain->imageCount;
  std::vector<vulkan::ImageInfo> renderImageCreateInfos;
  std::vector<vulkan::BufferInfo> renderBufferCreateInfos;

  for (auto& resource : textureBlackboard) {
    uint32_t width = getImageSize(
        resource.second->renderTextureInfo.sizeRelative, swapchain->imageWidth,
        resource.second->renderTextureInfo.sizeX);
    uint32_t height = getImageSize(
        resource.second->renderTextureInfo.sizeRelative, swapchain->imageHeight,
        resource.second->renderTextureInfo.sizeY);

    vulkan::ImageInfo info{
        .width = width,
        .height = height,
        .format = resource.second->renderTextureInfo.format,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = resource.second->renderTextureInfo.usage,
        .mipLevels = resource.second->renderTextureInfo.mipLevels,
        .arrayLayers = resource.second->renderTextureInfo.arrayLayers,
        .samples = resource.second->renderTextureInfo.samples,
        .memoryFlags = resource.second->renderTextureInfo.memoryFlags,
        .resourceLifespan = resource.second->resourceLifespan,
        .requireSampler = true,
        .requireImageView = true,
    };
    resource.second->resourceIndex = renderImageCreateInfos.size();
    renderImageCreateInfos.push_back(info);
  }

  internalRenderImages = device->createAliasedImages(renderImageCreateInfos);

  for (auto& resource : bufferBlackboard) {
    vulkan::BufferInfo info{
        .size = resource.second->renderBufferInfo.size,
        .usage = resource.second->renderBufferInfo.usage,
        .memoryFlags = resource.second->renderBufferInfo.memoryFlags,
        .requireMappedData =
            resource.second->renderBufferInfo.requireMappedData,
        .resourceLifespan = resource.second->resourceLifespan,
    };

    resource.second->resourceIndex = renderBufferCreateInfos.size();
    renderBufferCreateInfos.push_back(info);
  }

  internalRenderBuffers = device->createAliasedBuffers(renderBufferCreateInfos);
}

uint32_t RenderGraph::getImageSize(AttachmentSizeRelative sizeRelative,
                                   uint32_t swapchainSize, float size) {
  switch (sizeRelative) {
    case core_internal::rendering::AttachmentSizeRelative::SwapchainRelative:
      return static_cast<uint32_t>((float)swapchainSize * size);
    case core_internal::rendering::AttachmentSizeRelative::AbsoluteValue:
      return static_cast<uint32_t>(size);
    default:
      DEBUG_ERROR("Missing getImageSize Implementation");
      return -1;
  }
}

void RenderGraph::generateDescriptorSets() {
  // Required Data for rendering:
  // Material / Textures
  //    Material-> Tex Index + Modifiers
  // Camera + OBJ Matrices
  // Other Draw Data -> Required Indices per model set by DrawIndexedIndirect

  // TODO: Test Solutions for allocating pools/pool sizes
  // also->if(vkAllocateDescriptorSets == error) Target New Pool

  {  // Generic Descriptor Set & Pool
    std::array<VkDescriptorPoolSize, 3> poolSizes = {
        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,         1024,
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         1024,
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1024};

    VkDescriptorPoolCreateInfo descriptorPoolCI{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT,
        .maxSets = 1024,
        .poolSizeCount = poolSizes.size(),
        .pPoolSizes = poolSizes.data(),
    };

    vkCreateDescriptorPool(device->logicalDevice, &descriptorPoolCI, nullptr,
                           &bindlessDescriptorPool);

    std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
        {0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1024, VK_SHADER_STAGE_ALL,
         nullptr},
        {1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1024, VK_SHADER_STAGE_ALL,
         nullptr},
        {2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1024,
         VK_SHADER_STAGE_ALL, nullptr},
    };

    std::array<VkDescriptorBindingFlags, 3> flags{
        VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT |
            VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT,
        VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT |
            VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT,
        VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT |
            VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT};

    VkDescriptorSetLayoutBindingFlagsCreateInfo flagInfo{
        .sType =
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
        .bindingCount = flags.size(),
        .pBindingFlags = flags.data(),
    };

    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCI{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = &flagInfo,
        .bindingCount = setLayoutBindings.size(),
        .pBindings = setLayoutBindings.data(),
    };

    VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device->logicalDevice,
                                                &descriptorSetLayoutCI, nullptr,
                                                &bindlessDesciptorLayout));

    VkDescriptorSetAllocateInfo descriptorSetAllocInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = bindlessDescriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts = &bindlessDesciptorLayout,
    };

    VK_CHECK_RESULT(vkAllocateDescriptorSets(
        device->logicalDevice, &descriptorSetAllocInfo, &bindlessDescriptor));
  }

  // Unique per pass:
  for (auto& pass : renderPasses) {
    auto& inAtt = pass->getInputAttachments();
    auto& inStr = pass->getInputStorage();

    std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings;

    uint32_t inAttCount = inAtt.size();
    for (uint32_t i = 0; i < inAttCount; i++) {
      setLayoutBindings.push_back(
          {i, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
           inAtt[i]->renderTextureInfo.shaderStageFlag, nullptr});
    }

    for (uint32_t i = 0; i < inStr.size(); i++) {
      setLayoutBindings.push_back(
          {inAttCount + i, inStr[i]->renderBufferInfo.type, 1,
           inStr[i]->renderBufferInfo.shaderStageFlag, nullptr});
    }

    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCI{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = setLayoutBindings.size(),
        .pBindings = setLayoutBindings.data(),
    };

    pass->createDescriptorSetLayout(device->logicalDevice,
                                    &descriptorSetLayoutCI);
    // Allocate Descriptor Sets (Requires pool/pools)
  }
}

// Baking and any process related to this is not well optimized, baking only
// happens when the pipeline is created/changed and I am OK with this being
// slow. I may come back to this eventually and optimize it, however my focus is
// on runtime performance
void RenderGraph::bake() {
  validateData();

  generateDependencyChain();
  generateResources();

  generateDescriptorSets();
  generateSemaphores();
  // printRenderGraph();
}
}  // namespace core_internal::rendering