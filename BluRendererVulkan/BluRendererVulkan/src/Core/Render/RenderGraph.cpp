#include "RenderGraph.hpp"

#include <iostream>

#include "ResourceManagement/ExternalResources/Debug.hpp"

namespace core_internal::rendering {
void RenderGraphPass::addAttachmentInput(const std::string& name) {
  auto tex = graph->getTexture(name);
  tex->registerPass(index);

  inputTextureAttachments.push_back(tex);
}

RenderTextureResource* RenderGraphPass::addColorOutput(
    const std::string& name, const AttachmentInfo& info) {
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

void RenderGraphPass::set_GetCommandBuffer(std::function<void()> callback) {
  getCommandBuffer_cb = std::move(callback);
}

void RenderGraphPass::setQueue(const RenderGraphQueueFlags& queue) {
  this->queue = queue;
}

RenderGraphQueueFlags& RenderGraphPass::getQueue() { return queue; }

std::vector<RenderTextureResource*>& RenderGraphPass::getOutputAttachments() {
  return outputColorAttachments;
}

RenderGraphPass* RenderGraph::addPass(const std::string& name,
                                      const VkPipelineStageFlags flag) {
  return nullptr;
}

RenderTextureResource* RenderGraph::createTexture(const std::string& name,
                                                  const AttachmentInfo& info) {
  auto newTex = new RenderTextureResource();
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
  auto newBuf = new RenderBufferResource();
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
  if (renderGraphPasses.size() == 0) {
    DEBUG_ERROR("RenderGraph has no set Render Passes");
  }

  if (textureBlackboard.empty() && bufferBlackboard.empty()) {
    DEBUG_ERROR("No outputs defined in RenderGraph's set Render Passes");
  }

  for (auto& tex : textureBlackboard) {
    if (!tex.second->persistant && tex.second->usedInPasses.size() < 2) {
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
  for (uint32_t i = 0; i < renderGraphPasses.size(); i++) {
    // Start the search from every node
    dependencySearch(i, 0);
  }
}

void RenderGraph::dependencySearch(const uint32_t& passIndex,
                                   const uint32_t& depth) {
  // If depth is greater than number of render passes, it is a cyclic loop
  if (renderGraphPasses.size() < depth) {
    DEBUG_ERROR("Fatal: RenderGraph has a cyclic loop");
  }
  // If the dependencyLayer has already been set to a higher depth than the
  // depth we are currently on, we know the same will be true for the children
  // and can early exit this search
  if (renderGraphPasses[passIndex]->dependencyLayer > depth) {
    return;
  }
  renderGraphPasses[passIndex]->dependencyLayer = depth;
  auto& outputResources = renderGraphPasses[passIndex]->getOutputAttachments();

  for (uint32_t j = 0; j < outputResources.size(); j++) {
    if (outputResources[j]->minDependencyRange > depth) {
      outputResources[j]->minDependencyRange = depth;
    }
    if (outputResources[j]->maxDependencyRange < depth) {
      outputResources[j]->maxDependencyRange = depth;
    }

    auto& passes = outputResources[j]->usedInPasses;
    for (uint32_t k = 0; k < passes.size(); k++) {
      dependencySearch(passes[k], depth + 1);
    }
  }
}

void RenderGraph::generateResourceBuckets() {
  for (auto& resource : textureBlackboard) {
    if (resource.second->persistant) {
      resource.second->resourceIndex = persistantTextureBucket.size();
      persistantTextureBucket.push_back(TextureResourceReservation());
    } else {
      bool resourceAllocated = false;
      for (uint32_t texId = 0; texId < textureBucket.size(); texId++) {
        auto& tex = textureBucket[texId];
        if (tex.texInfo != resource.second->renderTextureInfo) continue;
        if (tex.dependencyReservation.size() <
            resource.second->maxDependencyRange)
          tex.dependencyReservation.insert(
              tex.dependencyReservation.end(), false,
              resource.second->maxDependencyRange -
                  tex.dependencyReservation.size());

        // If resource already reserved, continue
        for (uint32_t i = resource.second->minDependencyRange;
             i < resource.second->maxDependencyRange; i++) {
          if (tex.dependencyReservation[i]) continue;
        }

        // If resource is free, claim for requested range
        for (uint32_t i = resource.second->minDependencyRange;
             i < resource.second->maxDependencyRange; i++) {
          tex.dependencyReservation[i] = true;
        }

        resource.second->resourceIndex = texId;
        resourceAllocated = true;
        break;
      }

      if (!resourceAllocated) {
        resource.second->resourceIndex = textureBucket.size();
        textureBucket.push_back(
            TextureResourceReservation(resource.second->renderTextureInfo,
                                       resource.second->minDependencyRange,
                                       resource.second->maxDependencyRange));
      }
    }
  }

  for (auto& resource : bufferBlackboard) {
    if (resource.second->persistant) {
      resource.second->resourceIndex = persistantBufferBucket.size();
      persistantBufferBucket.push_back(BufferResourceReservation());
    } else {
      bool resourceAllocated = false;
      for (uint32_t bufId = 0; bufId < bufferBucket.size(); bufId++) {
        auto& buf = bufferBucket[bufId];
        if (buf.bufInfo != resource.second->renderBufferInfo) continue;
        if (buf.dependencyReservation.size() <
            resource.second->maxDependencyRange)
          buf.dependencyReservation.insert(
              buf.dependencyReservation.end(), false,
              resource.second->maxDependencyRange -
                  buf.dependencyReservation.size());

        // If resource already reserved, continue
        for (uint32_t i = resource.second->minDependencyRange;
             i < resource.second->maxDependencyRange; i++) {
          if (buf.dependencyReservation[i]) continue;
        }

        // If resource is free, claim for requested range
        for (uint32_t i = resource.second->minDependencyRange;
             i < resource.second->maxDependencyRange; i++) {
          buf.dependencyReservation[i] = true;
        }

        resource.second->resourceIndex = bufId;
        resourceAllocated = true;
        break;
      }

      if (!resourceAllocated) {
        resource.second->resourceIndex = bufferBucket.size();
        bufferBucket.push_back(
            BufferResourceReservation(resource.second->renderBufferInfo,
                                      resource.second->minDependencyRange,
                                      resource.second->maxDependencyRange));
      }
    }
  }
}

void RenderGraph::generateResources() {
  auto& fifCount = swapchain->imageCount;

  for (auto& buf : persistantBufferBucket) {
    device->createBuffer(buf.bufInfo.usage, buf.bufInfo.size,
                         buf.bufInfo.memory,
                         (buf.bufInfo.forceSingleInstance ? 1 : fifCount));
  }
}

// Output should resemble
// Buffer "BufferName"
// BufferSize X
// BufferUsage X
// BufferReservation
// [X] [X] [X] [ ] [X] [X] [ ]
// or
// 0-2, 4-5
// Texture Should be similar
void RenderGraph::printRenderGraph() {}

// Baking and any process related to this is not well optimized, baking only
// happens when the pipeline is changed and I am OK with this being slow. I
// may come back to this eventually and optimize it, however my focus is on
// runtime performance
void RenderGraph::bake() {
  validateData();

  // This leaves us with each pass being assigned a Dependency Layer, passes
  // with the same dependency layer can executed arbitrarily (Optimize 0_0)
  // Memory lifetime is tracked by dependency layer. Dependency layers have a
  // few assumptions: Won't write to the same resource, Each created resource
  // only exists for the current frame (This is not true for TAA, maybe
  // reserve frame permanent texture, create new TAA pass reading from old,
  // once done copy new tex to old tex? Or maybe pingpong 2 textures at the
  // cost of a little VRAM, but saving the copy time)
  generateDependencyChain();
  generateResourceBuckets();
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
  generateResources();

  generateMemoryBarriers();
  generateDescriptorSets();

  // Smart Descriptor Set Creation? Hash sets for reuse? or a descriptor set
  // just for externally managed resources. Could have one for
  // models+luts(Maybe seperate?) Textures are frag only, buffers could be
  // Vert | Frag | Both
}
}  // namespace core_internal::rendering