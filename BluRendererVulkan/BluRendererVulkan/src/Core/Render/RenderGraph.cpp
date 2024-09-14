#include "RenderGraph.hpp"

#include <iostream>

#include "../../libraries/GLTF/tiny_gltf.h"
#include "../Tools/Debug.hpp"
#include "ResourceManagement/ExternalResources/VulkanglTFModel.h"

namespace core_internal::rendering {
RenderGraphPass::RenderGraphPass(RenderGraph* rg) { graph = rg; }

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
// So drawing needs to be quite versatile to handle all sorts of variations
// Depth:
//  as Input: LAYOUT: PREV -> READ ONLY
//  as Output: UNDEF -> READONLY - (Maybe seperate stencil?)
// Color:
//  as Input: LAYOUT: PREV -> READ ONLY
//  as Output: LAYOUT: UNDEF -> COLOR OPTIMAL: It must be from UNDEF to clear
//  previous "junk" data as aliased resources are filled bad data
// ATTACHMENTS----------------
// Technically attachments could be baked I think, but maybe having it dynamic
// works:
// PROS:
//  Allows for on the fly changes without re-baking
// CONS:
//  Slower per frame CPU performance
// Solution:
// This will remain dynamic until it is rendering successfully, at that point I
// will re-evaluate based on profiling
// RENDERING-------------------
// Because DrawCommands are generated on GPU intead of on CPU, we could
// generate draw commands for different cases and reuse them
// Examples could be: Camera Occluded Draws, Shadow draws, UI draws? (UI should
// be capped at X FPS & probably recorded CPU side for simplicity)
// TODO: Support indexed resources (IE per FIF or hardcap at 2(I think this is
// best but need to investigate/profile to confirm beliefs))
void RenderGraphPass::draw(VkCommandBuffer buf) {
  if (drawType ==
      core_internal::rendering::RenderGraph::DrawType::DRAW_TYPE_COMPUTE) {
    vkCmdBindPipeline(buf, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
    vkCmdBindDescriptorSets(buf, VK_PIPELINE_BIND_POINT_COMPUTE,
                            computePipelineLayout, 0, 1,
                            &computeDescriptorSets[i], 0, 0);
    vkCmdDispatch(buf, 256, 1, 1);
    return;
  }

  std::vector<VkRenderingAttachmentInfoKHR> attachments;
  VkRenderingAttachmentInfoKHR depthStencilAttachment;
  uint32_t width = 0;
  uint32_t height = 0;

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
      .renderArea = {.extent{.width = width, .height = height}},
      .layerCount = 1,
      .viewMask = 0,  // Multiview
      .colorAttachmentCount = attachments.size(),
      .pColorAttachments = attachments.data(),
      .pDepthAttachment = &depthStencilAttachment,
      .pStencilAttachment = &depthStencilAttachment,
  };

  vkCmdBeginRenderingKHR(buf, &renderpassInfo);

  switch (drawType) {
    case core_internal::rendering::RenderGraph::DrawType::
        DRAW_TYPE_CAMERA_OCCLUDED_OPAQUE:
    case core_internal::rendering::RenderGraph::DrawType::
        DRAW_TYPE_CAMERA_OCCLUDED_TRANSLUCENT:
      vkCmdDrawIndexedIndirect(buf, graph->getDrawBuffer(drawType).buffer, 0, 1,
                               sizeof(float));  // TODO: Fill with non junk
      break;
    case core_internal::rendering::RenderGraph::DrawType::
        DRAW_TYPE_FULLSCREEN_TRIANGLE:
      vkCmdDraw(buf, 3, 1, 0, 0);  // Vertices generated in VertexShader
      break;
    case core_internal::rendering::RenderGraph::DrawType::
        DRAW_TYPE_CUSTOM_OCCLUSION:
      break;
    case core_internal::rendering::RenderGraph::DrawType::
        DRAW_TYPE_CPU_RECORDED:
      recordCommandBuffer_cb(buf);
      break;
    default:
      DEBUG_ERROR("Missing Draw Type implementation");
      break;
  }

  vkCmdEndRenderingKHR(buf);
}

void RenderGraph::clearModels() {}

// Models are loaded individually to return relevent rendering information to
// scene models
// Mesh->Primitives that contains a mesh:
//
uint32_t RenderGraph::registerModel(core::engine::components::Model* model) {
  if (model->filePath.empty()) {
    // If model is created without a filepath, this is empty meant to hold
    // components, it should be handled engine side and should never make it
    // here
    DEBUG_WARNING("Model has no filepath");
    return;
  }

  if (modelBlackboard.contains(model->filePath)) {
    auto modelInfoID = modelBlackboard[model->filePath];

    if (emptyModelSlots.empty()) {
      auto modelID = models.size();
      models.push_back(modelInfoID);
      return modelID;
    } else {
      auto modelID = emptyModelSlots.front();
      emptyModelSlots.pop();
      return modelID;
    }
  } else {
    vkglTF::Model newModel;
    newModel.loadFromFile(model->filePath);

    // if buffer can fit memory, send data
    // else allocate more memory?
  }
}

// size_t vertexBufferSize = vertexCount * sizeof(Vertex);
// size_t indexBufferSize = indexCount * sizeof(uint32_t);

// assert(vertexBufferSize > 0);

// struct StagingBuffer {
//   VkBuffer buffer;
//   VkDeviceMemory memory;
// } vertexStaging, indexStaging;

//// Create staging buffers
//// Vertex data
// VK_CHECK_RESULT(device->createBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
//                                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
//                                          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
//                                      vertexBufferSize,
//                                      &vertexStaging.buffer,
//                                      &vertexStaging.memory,
//                                      loaderInfo.vertexBuffer));
//// Index data
// if (indexBufferSize > 0) {
//   VK_CHECK_RESULT(
//       device->createBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
//                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
//                                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
//                            indexBufferSize, &indexStaging.buffer,
//                            &indexStaging.memory, loaderInfo.indexBuffer));
// }

//// Create device local buffers
//// Vertex buffer
// VK_CHECK_RESULT(device->createBuffer(
//     VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
//     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBufferSize,
//     &vertices.buffer, &vertices.memory));
//// Index buffer
// if (indexBufferSize > 0) {
//   VK_CHECK_RESULT(device->createBuffer(
//       VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
//       VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBufferSize,
//       &indices.buffer, &indices.memory));
// }

//// Copy from staging buffers
// VkCommandBuffer copyCmd =
//     device->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

// VkBufferCopy copyRegion = {};

// copyRegion.size = vertexBufferSize;
// vkCmdCopyBuffer(copyCmd, vertexStaging.buffer, vertices.buffer, 1,
//                 &copyRegion);

// if (indexBufferSize > 0) {
//   copyRegion.size = indexBufferSize;
//   vkCmdCopyBuffer(copyCmd, indexStaging.buffer, indices.buffer, 1,
//                   &copyRegion);
// }

// device->flushCommandBuffer(copyCmd, transferQueue, true);

// vkDestroyBuffer(device->logicalDevice, vertexStaging.buffer, nullptr);
// vkFreeMemory(device->logicalDevice, vertexStaging.memory, nullptr);
// if (indexBufferSize > 0) {
//   vkDestroyBuffer(device->logicalDevice, indexStaging.buffer, nullptr);
//   vkFreeMemory(device->logicalDevice, indexStaging.memory, nullptr);
// }
// }

RenderGraphPass* RenderGraph::addPass(const std::string& name,
                                      const DrawType& drawType) {
  RenderGraphPass* newPass = new RenderGraphPass(this);
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

  if (textureBlackboard.empty() && bufferBlackboard.empty()) {
    DEBUG_ERROR("No outputs defined in RenderGraph's set Render Passes");
  }

  for (auto& tex : textureBlackboard) {
    if (!tex.second->renderTextureInfo.isExternal &&
        tex.second->usedInPasses.size() < 2) {
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
  std::vector<vulkan::ImageInfo> imageCreateInfos;
  std::vector<vulkan::ImageInfo> renderImageCreateInfos;
  std::vector<vulkan::BufferInfo> bufferCreateInfos;
  std::vector<vulkan::BufferInfo> renderBufferCreateInfos;

  for (auto& resource : textureBlackboard) {
    if (resource.second->renderTextureInfo.isExternal) {
      uint32_t width = getImageSize(
          resource.second->renderTextureInfo.sizeRelative,
          swapchain->imageWidth, resource.second->renderTextureInfo.sizeX);
      uint32_t height = getImageSize(
          resource.second->renderTextureInfo.sizeRelative,
          swapchain->imageHeight, resource.second->renderTextureInfo.sizeY);

      vulkan::ImageInfo info{
          .width = width,
          .height = height,
          .format = resource.second->renderTextureInfo.format,
          .tiling = VK_IMAGE_TILING_OPTIMAL,
          .usage = resource.second->renderTextureInfo.usage,
          .mipLevels = resource.second->renderTextureInfo.mipLevels,
          .arrayLayers = resource.second->renderTextureInfo.arrayLayers,
          .samples = resource.second->renderTextureInfo.samples,
          .resourceLifespan = resource.second->resourceLifespan,
          .requireSampler = resource.second->renderTextureInfo.requireSampler,
          .requireImageView =
              resource.second->renderTextureInfo.requireImageView,
          .requireMappedData =
              resource.second->renderTextureInfo.requireMappedData,
      };
      resource.second->resourceIndex = imageCreateInfos.size();
      imageCreateInfos.push_back(info);
    } else {
      uint32_t width = getImageSize(
          resource.second->renderTextureInfo.sizeRelative,
          swapchain->imageWidth, resource.second->renderTextureInfo.sizeX);
      uint32_t height = getImageSize(
          resource.second->renderTextureInfo.sizeRelative,
          swapchain->imageHeight, resource.second->renderTextureInfo.sizeY);

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
  }

  for (auto& image : imageCreateInfos) {
    renderImages.push_back(device->createImage(image, false));
  }

  internalRenderImages = device->createAliasedImages(renderImageCreateInfos);

  for (auto& resource : bufferBlackboard) {
    if (resource.second->renderBufferInfo.isExternal) {
      vulkan::BufferInfo info{
          .size = resource.second->renderBufferInfo.size,
          .usage = resource.second->renderBufferInfo.usage,
          .memoryFlags = resource.second->renderBufferInfo.memoryFlags,
          .requireMappedData =
              resource.second->renderBufferInfo.requireMappedData,
          .resourceLifespan = resource.second->resourceLifespan,
      };

      resource.second->resourceIndex = bufferCreateInfos.size();
      bufferCreateInfos.push_back(info);
    } else {
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
  }

  for (auto& buffer : bufferCreateInfos) {
    renderBuffers.push_back(device->createBuffer(buffer, false));
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
  // Required Data for rendering Models found in RegisteredModel:
  // VkBuffer:
  //
  // VkImage:
  //
  // VkBuffer:
  //
  //
  // Generic Descriptor Sets
  // registerModel(Mesh, Model?)
  // MESH:
  //  Mesh->
  // MODEL:
  //  Textures->Save Indices once registered
  // AddDescriptorSet()
  modelDescriptorSet;
  // Unique per pass:
  for (auto& pass : renderPasses) {
    pass->
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

  // TODO
  generateDescriptorSets();
  generateSemaphores();
  // printRenderGraph();

  // GPU RESOURCES
  // BUFFERS
  //  Buffers Refer to raw data stored on the GPU
  // IMAGES
  //   Images refer to what renderpasses use as output/input
  // TEXTURES
  //   referes to what models use for colours

  // For Resource Generation
  // Needs:
  // One resource Per Frame
  // Persitant Mapped Data needs to be accessible
  // Persitant data will need to be resizeable
  // Non Persistant data not accessible
  // Foreach resource, device->create()
  // For Textures, support different filetypes(ktx, dds, png & jpeg)
  // For Textures, support Cubemaps, Tex2D

  // Smart Descriptor Set Creation? Hash sets for reuse? or a descriptor set
  // just for externally managed resources. Could have one for
  // models+luts(Maybe seperate?) Textures are frag only, buffers could be
  // Vert | Frag | Both
}
}  // namespace core_internal::rendering