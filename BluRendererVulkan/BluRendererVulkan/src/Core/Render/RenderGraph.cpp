#include "RenderGraph.hpp"

#include <iostream>
#include <ranges>

#include "../../libraries/GLTF/tiny_gltf.h"
#include "../Tools/Debug.hpp"
#include "ResourceManagement/ExternalResources/VulkanglTFModel.h"

// RenderGraphPass
namespace core_internal::rendering::rendergraph {
RenderGraphPass::RenderGraphPass(RenderGraphPassType passType, glm::vec3 size,
                                 std::vector<std::string> shaders)
    : passType(passType), size(size), shaders(shaders) {}

RenderGraphPass::~RenderGraphPass() {}

void RenderGraphPass::validateData() {
  if (outputImages.size() == 0 && outputBuffers.size() == 0) {
    DEBUG_ERROR("RenderPass has no set outputs");
  }
  glm::vec2 imgSize = {0, 0};

  for (auto& output : outputImages) {
    if (imgSize == glm::vec2(0, 0)) {
      imgSize = output.size;
    }
    if (imgSize != output.size) {
      DEBUG_ERROR("Output Image attachments have different dimensions");
    }
  }

  if (shaders.size() == 0) {
    DEBUG_ERROR("No Shaders added to pass");
  }
}

glm::vec2 RenderGraphPass::getSize() { return size; }

void RenderGraphPass::addInput(std::string resourceName) {
  inputs.push_back(resourceName);
}

void RenderGraphPass::addOutput(std::string name, VkBufferUsageFlagBits usage,
                                VkDeviceSize size) {
  outputBuffers.push_back({name, usage, size});
}

void RenderGraphPass::addOutput(std::string name, VkImageUsageFlagBits usage) {
  outputImages.push_back({name, usage, size});
}

RenderGraph::RenderGraph(core_internal::rendering::VulkanDevice* vulkanDevice,
                         glm::ivec2 targetSize)
    : vulkanDevice(vulkanDevice), targetSize(targetSize) {}

RenderGraph::~RenderGraph() {}

void RenderGraph::toggleDebugUI() {
  // Setup Custom ImGUI debug UI including:
  // RenderGraph DependencyChains & Q Submits
}

void RenderGraph::setTargetSize(glm::ivec2 targetSize) {
  this->targetSize = targetSize;
}

void RenderGraph::setFinalOutput(std::string finalOutput) {
  if (bakedInfo.imageBlackboard.count(finalOutput) == 0) {
    DEBUG_ERROR("Specified final output not found");
  }
  bakedInfo.finalOutput = finalOutput;
}

void RenderGraph::bake() {
#ifdef DEBUG_RENDERGRAPH
  auto tStart = std::chrono::high_resolution_clock::now();
#endif
  getRenderpassData();
  validateData();
  generateDependencyChain();
  generateBufferResourceReservations();
  generateImageResourceReservations();
  generateResources();
  generateDescriptorSets();
#ifdef DEBUG_RENDERGRAPH
  auto tEnd = std::chrono::duration<double, std::milli>(
                  std::chrono::high_resolution_clock::now() - tStart)
                  .count();
  std::cout << "Baking the Rendergraph took " << tEnd << " ms" << std::endl;
#endif
}

void RenderGraph::registerExternalData(
    std::string name, core_internal::rendering::Buffer* buffer) {
  auto [it, success] =
      bakedInfo.bufferBlackboard.try_emplace(name, BufferInfo(0, buffer));

  if (!success) {
    DEBUG_ERROR("Buffer with name: << " + name +
                " >> has already registered to RenderGraph");
  }
}

void RenderGraph::registerExternalData(std::string name,
                                       core_internal::rendering::Image* image) {
  auto [it, success] =
      bakedInfo.imageBlackboard.try_emplace(name, ImageInfo(0, image));

  if (!success) {
    DEBUG_ERROR("Image with name: << " + name +
                " >> has already registered to RenderGraph");
  }
}

RenderGraphPass* RenderGraph::addGraphicsPass(
    std::vector<std::string> shaders) {
  RenderGraphPass* newPass = new RenderGraphPass(
      RenderGraphPass::RenderGraphPassType::Graphics, {1, 1, 0}, shaders);

  buildInfo.rgPasses.push_back(newPass);
  return newPass;
}

RenderGraphPass* RenderGraph::addComputePass(std::string computeShader,
                                             glm::ivec3 dispatchGroup) {
  RenderGraphPass* newPass =
      new RenderGraphPass(RenderGraphPass::RenderGraphPassType::Compute,
                          dispatchGroup, {computeShader});

  buildInfo.rgPasses.push_back(newPass);
  return newPass;
}

void RenderGraph::getRenderpassData() {
  for (size_t i = 0; i < buildInfo.rgPasses.size(); i++) {
    auto& bufOut = buildInfo.rgPasses[i]->getBufferOutputs();
    auto& imgOut = buildInfo.rgPasses[i]->getImageOutputs();

    for (auto& buf : bufOut) {
      auto [it, success] = bakedInfo.bufferBlackboard.try_emplace(
          buf.name, BufferInfo(i, nullptr, buf));

      if (!success) {
        DEBUG_ERROR("Buffer with name: << " + buf.name +
                    " >> has already registered to RenderGraph");
      }
    }

    for (auto& img : imgOut) {
      auto [it, success] = bakedInfo.imageBlackboard.try_emplace(
          img.name, ImageInfo(i, nullptr, img));

      if (!success) {
        DEBUG_ERROR("Image with name: << " + img.name +
                    " >> has already registered to RenderGraph");
      }
    }
  }
  for (size_t i = 0; i < buildInfo.rgPasses.size(); i++) {
    auto& inputs = buildInfo.rgPasses[i]->getInputs();
    for (auto& input : inputs) {
      auto bufIt = bakedInfo.bufferBlackboard.find(input);
      if (bufIt != bakedInfo.bufferBlackboard.end()) {
        bufIt->second.usedIn.push_back(i);
      }

      auto imgIt = bakedInfo.imageBlackboard.find(input);
      if (imgIt != bakedInfo.imageBlackboard.end()) {
        imgIt->second.usedIn.push_back(i);
      }
    }
  }
}

void RenderGraph::validateData() {
  if (buildInfo.rgPasses.size() == 0) {
    DEBUG_ERROR("RenderGraph has no set Render Passes");
  }

  for (auto& pass : buildInfo.rgPasses) {
    pass->validateData();
  }
}

void RenderGraph::dependencySearch(const uint32_t& passIndex,
                                   const uint32_t& depth) {
  // If depth is greater than number of render passes, it is a cyclic loop
  if (buildInfo.rgPasses.size() < depth) {
    DEBUG_ERROR("RenderGraph has a cyclic loop");
  }
  // If the dependencyLayer has already been set to a higher depth than the
  // depth we are currently on, we know the same will be true for the children
  // and can early exit this search
  if (buildInfo.rgDependencyLayer[passIndex] > depth) {
    return;
  }

  buildInfo.rgDependencyLayer[passIndex] = depth;

  auto& outputImages = buildInfo.rgPasses[passIndex]->getImageOutputs();
  for (uint32_t j = 0; j < outputImages.size(); j++) {
    bakedInfo.imageBlackboard[outputImages[j].name].resourceLifespan |=
        1 << depth;

    auto& passes = bakedInfo.imageBlackboard[outputImages[j].name].usedIn;

    for (uint32_t k = 0; k < passes.size(); k++) {
      dependencySearch(passes[k], depth + 1);
    }
  }

  auto& outputBuffers = buildInfo.rgPasses[passIndex]->getBufferOutputs();
  for (uint32_t j = 0; j < outputBuffers.size(); j++) {
    bakedInfo.bufferBlackboard[outputBuffers[j].name].resourceLifespan |=
        1 << depth;

    auto& passes = bakedInfo.bufferBlackboard[outputBuffers[j].name].usedIn;

    for (uint32_t k = 0; k < passes.size(); k++) {
      dependencySearch(passes[k], depth + 1);
    }
  }
}

void RenderGraph::generateDependencyChain() {
  buildInfo.rgDependencyLayer.resize(buildInfo.rgPasses.size());

  for (uint32_t i = 0; i < buildInfo.rgPasses.size(); i++) {
    // Start the search from every node
    dependencySearch(i, 0);
  }
}

// TODO: SUPPORT FIF
void RenderGraph::generateResources() {
  for (auto& bufReservation : buildInfo.bufferReservations) {
    VmaAllocation newAlloc{};

    VmaAllocationCreateInfo allocCI{
        .preferredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    };

    vulkanDevice->allocateMemory(&newAlloc, allocCI, bufReservation.memReqs);

    for (auto& bufInd : bufReservation.usedByResources) {
      vulkanDevice->bindMemory(bakedInfo.bufferBlackboard[bufInd].buf,
                               newAlloc);
    }
  }

  for (auto& imgReservation : buildInfo.imageReservations) {
    VmaAllocation newAlloc{};

    VmaAllocationCreateInfo allocCI{
        .preferredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    };

    vulkanDevice->allocateMemory(&newAlloc, allocCI, imgReservation.memReqs);

    for (auto& imgInd : imgReservation.usedByResources) {
      vulkanDevice->bindMemory(bakedInfo.imageBlackboard[imgInd].img, newAlloc);

      VkImageViewCreateInfo viewCI{
          .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
          .flags = 0,
          .image = bakedInfo.imageBlackboard[imgInd].img->image,
          .viewType = VK_IMAGE_VIEW_TYPE_2D,
          .format = vulkanDevice->getImageFormat(
              bakedInfo.imageBlackboard[imgInd].imgInfo.usage),
          .subresourceRange{
              vulkanDevice->getImageAspectMask(
                  bakedInfo.imageBlackboard[imgInd].imgInfo.usage,
                  vulkanDevice->getImageFormat(
                      bakedInfo.imageBlackboard[imgInd].imgInfo.usage)),
              0, 1, 0, 1},
      };

      vulkanDevice->createImageView(bakedInfo.imageBlackboard[imgInd].img,
                                    viewCI);

      VkSamplerCreateInfo samplerCI{
          .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
          .magFilter = VK_FILTER_LINEAR,
          .minFilter = VK_FILTER_LINEAR,
          .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
          .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
          .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
          .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
          .mipLodBias = 0.0f,
          .maxAnisotropy = 1.0f,
          .minLod = 0.0f,
          .maxLod = 1.0f,
          .borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
      };
      vulkanDevice->createImageSampler(bakedInfo.imageBlackboard[imgInd].img,
                                       samplerCI);
    }
  }
}

void RenderGraph::generateBufferResourceReservations() {
  // First, sort based on size
  // Then reserve RenderGraph Resources based on resource lifespans & size
  std::multimap<VkDeviceSize, std::string> sortedBufferBlackboard;
  for (auto& buf : bakedInfo.bufferBlackboard) {
    sortedBufferBlackboard.insert(
        std::make_pair(buf.second.bufInfo.size, buf.first));
  }

  for (auto bufferData = sortedBufferBlackboard.rbegin();
       bufferData != sortedBufferBlackboard.rend(); ++bufferData) {
    auto& buf = bakedInfo.bufferBlackboard[bufferData->second];

    // If the buffer already exists(externally managed resource) continue
    if (buf.buf != nullptr) continue;

    bakedInfo.bakedVRAMBufferSizeAbsolute += bufferData->first;
    unsigned long firstUse, lastUse;
    BitScanForward(&firstUse, buf.resourceLifespan);
    BitScanReverse(&lastUse, buf.resourceLifespan);

    bool success = true;
    for (auto& reservation : buildInfo.bufferReservations) {
      if (bufferData->first < reservation.memReqs.size) continue;

      unsigned long resourceMemReservation = reservation.memoryReservation;
      success = true;

      for (unsigned long i = firstUse; i < lastUse; i++) {
        if ((resourceMemReservation & (1 << i)) != 0) {
          success = false;
          break;
        } else {
          resourceMemReservation |= (1 << i);
        }
      }

      if (success) {
        reservation.memoryReservation = resourceMemReservation;
        reservation.usedByResources.push_back(bufferData->second);
        break;
      }
    }

    // If memory was unable to be reserved in existing reservations, create
    // new
    if (!success) {
      buildInfo.bufferReservations.push_back(RenderResource(
          bufferData->second, firstUse, lastUse, {bufferData->first, 0, 0}));
      bakedInfo.bakedVRAMBufferSizeActual += bufferData->first;
    }
  }
}

void RenderGraph::generateImageResourceReservations() {
  std::multimap<VkDeviceSize, std::string> sortedImageBlackboard;
  for (auto& image : bakedInfo.imageBlackboard) {
    if (image.second.img == nullptr) continue;

    VkImageCreateInfo imageCI{
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .flags = VK_IMAGE_CREATE_ALIAS_BIT,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = vulkanDevice->getImageFormat(image.second.imgInfo.usage),
        .extent = {image.second.imgInfo.size.x * targetSize.x,
                   image.second.imgInfo.size.y * targetSize.y, 1},
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = image.second.imgInfo.usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    image.second.img = new Image();

    vulkanDevice->createImage(image.second.img, imageCI);

    sortedImageBlackboard.insert(
        std::make_pair(image.second.img->size, image.first));
  }

  for (auto imageData = sortedImageBlackboard.rbegin();
       imageData != sortedImageBlackboard.rend(); ++imageData) {
    auto& img = bakedInfo.imageBlackboard[imageData->second];

    bakedInfo.bakedVRAMImageSizeAbsolute += imageData->first;
    unsigned long firstUse, lastUse;
    BitScanForward(&firstUse, img.resourceLifespan);
    BitScanReverse(&lastUse, img.resourceLifespan);

    bool success = true;
    for (auto& reservation : buildInfo.imageReservations) {
      if (imageData->first < reservation.memReqs.size ||
          !(img.img->memReqs.memoryTypeBits &
            reservation.memReqs.memoryTypeBits)) {
        continue;
      }

      unsigned long resourceMemReservation = reservation.memoryReservation;
      success = true;

      for (unsigned long i = firstUse; i < lastUse; i++) {
        if (resourceMemReservation & (1 << i)) {
          success = false;
          break;
        } else {
          resourceMemReservation |= (1 << i);
        }
      }

      if (success) {
        reservation.memoryReservation = resourceMemReservation;
        reservation.usedByResources.push_back(imageData->second);
        reservation.memReqs.memoryTypeBits &=
            reservation.memReqs.memoryTypeBits;
        if (reservation.memReqs.alignment < img.img->memReqs.alignment) {
          reservation.memReqs.alignment = img.img->memReqs.alignment;
        }
        break;
      }
    }

    // If memory was unable to be reserved in existing reservations, create
    // new
    if (!success) {
      buildInfo.imageReservations.push_back(RenderResource(
          imageData->second, firstUse, lastUse, img.img->memReqs));
      bakedInfo.bakedVRAMImageSizeActual += imageData->first;
    }
  }
}  // namespace core_internal::rendering::rendergraph

void RenderGraph::generateDescriptorSets() {
  // Descriptor Set Per Pass
  // Maybe flags for generic passes?
  // Or maybe hash sets

  // Should "build" to baked passes
  // Store Baked Passes based on dependencyIndex
  // Result of rendering should be multiple VkCommandBuffers
  // 1 thread per dependencyLayer would load each thread, and
  // syncing threads with Q submits would keep the GPU pretty busy
  // This scales well with more threads, but singular big workloads would be
  // pretty slow

  for (auto& pass : buildInfo.rgPasses) {
    RenderPass newBakedPass{};

    auto& imgs = pass->getImageOutputs();
    auto& bufs = pass->getBufferOutputs();

    newBakedPass.size = pass->getSize();
    newBakedPass.size *= targetSize;
    newBakedPass.imgCount = imgs.size();
    newBakedPass.bufCount = bufs.size();

    newBakedPass.images = new Image*[newBakedPass.imgCount];
    newBakedPass.buffers = new Buffer*[newBakedPass.bufCount];

    for (uint32_t i = 0; i < newBakedPass.imgCount; i++) {
      newBakedPass.images[i] = bakedInfo.imageBlackboard[imgs[i].name].img;
    }

    for (uint32_t i = 0; i < newBakedPass.bufCount; i++) {
      newBakedPass.buffers[i] = bakedInfo.bufferBlackboard[bufs[i].name].buf;
    }
  }
}
}  // namespace core_internal::rendering::rendergraph