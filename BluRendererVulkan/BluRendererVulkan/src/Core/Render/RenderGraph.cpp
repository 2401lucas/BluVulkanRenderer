#include "RenderGraph.hpp"

#include <iostream>

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

void RenderGraphPass::checkQueue(const uint32_t& passID,
                                 const RenderGraphQueueFlags& passQueue) {
  if (queue != passQueue) {
    passesToSync.push_back(passID);
  }
}

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
      blackboard.try_emplace(name, static_cast<RenderResource*>(newTex));

  if (!success) {
    std::cerr << "Fatal: << " << name
              << " >> has already registered to RenderGraph";
  }

  return newTex;
}

RenderBufferResource* RenderGraph::createBuffer(const std::string& name,
                                                const BufferInfo& info) {
  auto newBuf = new RenderBufferResource();
  auto [it, success] =
      blackboard.try_emplace(name, static_cast<RenderResource*>(newBuf));

  if (!success) {
    std::cerr << "Fatal: << " << name
              << " >> has already registered to RenderGraph";
  }

  return newBuf;
}

RenderTextureResource* RenderGraph::getTexture(const std::string& name) {
  if (auto res = blackboard.find(name); res != blackboard.end()) {
    return static_cast<RenderTextureResource*>(res->second);
  } else {
    std::cerr << "Fatal: Texture << " << name
              << " >> has not been registered to RenderGraph, but is being "
                 "referenced";
  }
}

RenderBufferResource* RenderGraph::getBuffer(const std::string& name) {
  if (auto res = blackboard.find(name); res != blackboard.end()) {
    return static_cast<RenderBufferResource*>(res->second);
  } else {
    std::cerr << "Fatal: Buffer << " << name
              << " >> has not been registered to RenderGraph, but is being "
                 "referenced";
  }
}

void RenderGraph::validateData() {
  if (renderGraphPasses.size() == 0) {
    std::cerr << "Fatal: RenderGraph has not set Render Passes"
                 "\" in "
              << __FILE__ << " at line " << __LINE__ << "\n";
    exit(2401);
  }

  if (blackboard.empty()) {
    std::cerr << "Fatal: No outputs defined in RenderGraph's set Render Passes"
                 "\" in "
              << __FILE__ << " at line " << __LINE__ << "\n";
    exit(2401);
  }

  if (finalOutput == nullptr) {
    std::cerr << "Fatal: RenderGraph's finalOutput has not been set"
                 "\" in "
              << __FILE__ << " at line " << __LINE__ << "\n";
    exit(2401);
  }
}

// ok so what does this need to do
// - Check for Cyclic Dependencies
// - Assign Dependency Layer
// - 
// Final Output: RenderPasses sorted by Dependency Layer,
void RenderGraph::generateDependencyChain() {
  // Recursive search each child passes, assigning value based on depth
  // If currently assigned DependencyLayer is less than new DependencyLayer new
  // value is used
  for (uint32_t i = 0; i < renderGraphPasses.size(); i++) {
    // Start the search from every node
    dependencySearch(i, 0);
  }

  // Need to check for cyclic dependencies
  // Adjecency list is redundant because we track what textures are referenced
  // by what passes Creates adjecency list based on outputs, tracking if a
  // passes queue is different than it's predecessor
  for (uint32_t i = 0; i < renderGraphPasses.size(); i++) {
    auto& baseRenderPass = renderGraphPasses[i];
    auto& outputResources = baseRenderPass->getOutputAttachments();
    for (uint32_t j = 0; j < outputResources.size(); j++) {
      auto& passes = outputResources[j]->usedInPasses;
      for (uint32_t k = 0; k < passes.size(); k++) {
        auto& passId = passes[k];
        renderGraphPasses[passId]->checkQueue(i, baseRenderPass->getQueue());
      }
    }
  }

  // This leaves us with each pass being assigned a Dependency Layer, passes
  // with the same dependency layer can executed arbitrarily (Optimize 0_0)
  // Memory lifetime is tracked by dependency layer. Dependency layers have a
  // few assumptions: Won't write to the same resource, Each created resource
  // only exists for the current frame (This is not true for TAA, maybe reserve
  // frame permanent texture, create new TAA pass reading from old, once done
  // copy new tex to old tex? Or maybe pingpong 2 textures at the cost of a
  // little VRAM, but saving the copy time)
}

void RenderGraph::dependencySearch(const uint32_t& passIndex,
                                   const uint32_t& depth) {
  // If the dependencyLayer has already been set to a higher depth than the
  // depth we are currently on, we know the same will be true for the children
  // and can early exit this search
  if (renderGraphPasses[passIndex]->dependencyLayer > depth) {
    return;
  }
  renderGraphPasses[passIndex]->dependencyLayer = depth;
  auto& outputResources = renderGraphPasses[passIndex]->getOutputAttachments();

  for (uint32_t j = 0; j < outputResources.size(); j++) {
    auto& passes = outputResources[j]->usedInPasses;
    for (uint32_t k = 0; k < passes.size(); k++) {
      dependencySearch(passes[k], depth + 1);
    }
  }
}

// Some of the logic this will need:
// Smart Texture allocation for non frame persistant data:
// Request Texture of <Size, Format> for <range of time>, if another texture
// exists and is free during that time reserve the texture, else create a new
// texture. TextureIndex of that texture is saved, and is required when
// building descriptor sets for that pass.
// Should be persistant/static and dynamic memory
// Persistant/static still has the ability to be upated, but is
// expected to be updated less frequently. This data will persist thru frames
// and potentially RenderGraph updates, the size of this data is static
// requiring a realloc/copy if it changes.
// Dynamic is expected to be updated
// frequently, in the future could also auto manage/resize buffer without need
// to copy all info
void RenderGraph::generateResourceBuckets() {
  std::vector<ResourceReservation> buffers;
  std::vector<ResourceReservation> textures;

  // Get list sorted by size
  // Compare against list
  // Insert & Cache index
}

#ifdef DEBUG_RENDERGRAPH
void RenderGraph::printRenderGraph() {}
#endif  // DEBUG_RENDERGRAPH

// Baking and any process related to this is not well optimized, baking only
// happens when the pipeline is changed and I am OK with this being slow. I may
// come back to this eventually and optimize it, however my focus is on runtime
// performance
void RenderGraph::bake() {
  validateData();

  generateDependencyChain();
  // Only generate buckets after dependency chain has beeen generated
  // ID's do NOT tell the full story, they only specify the order that the pass
  // was requested in
  generateResourceBuckets();

  // Figure out where to put memory barriers
  //
  //
  // Smart Descriptor Set Creation? Hash sets for reuse? or a descriptor set
  // just for externally managed resources. Could have one for models+luts(Maybe
  // seperate?) Textures are frag only, buffers could be Vert | Frag | Both
}

RenderBufferResource::RenderBufferResource() : RenderResource(0) {}

RenderTextureResource::RenderTextureResource() : RenderResource(0) {}
}  // namespace core_internal::rendering