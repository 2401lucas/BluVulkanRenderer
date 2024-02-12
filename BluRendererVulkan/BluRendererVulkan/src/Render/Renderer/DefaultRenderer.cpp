#include "DefaultRenderer.h"

#include <stdexcept>

#include "../Descriptors/DescriptorUtils.h"
#include "../RenderPass/RenderPassUtils.h"

DefaultRenderer::DefaultRenderer(GLFWwindow* window,
                                 const VkApplicationInfo& appInfo,
                                 DeviceSettings deviceSettings,
                                 const SceneDependancies* sceneDependancies)
    : BaseRenderer(window, appInfo, deviceSettings) {
  VkAttachmentDescription colorAttachment =
      RenderPassUtils::createAttachmentDescription(
          swapchain->getSwapchainFormat(), device->getMipSampleCount(),
          VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE,
          VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE,
          VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
  VkAttachmentDescription depthAttachment =
      RenderPassUtils::createAttachmentDescription(
          device->findSupportedFormat(
              {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT,
               VK_FORMAT_D24_UNORM_S8_UINT},
              VK_IMAGE_TILING_OPTIMAL,
              VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT),
          device->getMipSampleCount(), VK_ATTACHMENT_LOAD_OP_CLEAR,
          VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_ATTACHMENT_LOAD_OP_DONT_CARE,
          VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_UNDEFINED,
          VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
  VkAttachmentDescription colorAttachmentResolve =
      RenderPassUtils::createAttachmentDescription(
          swapchain->getSwapchainFormat(), VK_SAMPLE_COUNT_1_BIT,
          VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_STORE,
          VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE,
          VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

  VkAttachmentReference colorAttachmentRef =
      RenderPassUtils::createAttachmentRef(
          0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
  VkAttachmentReference depthAttachmentRef =
      RenderPassUtils::createAttachmentRef(
          1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
  VkAttachmentReference colorAttachmentResolveRef =
      RenderPassUtils::createAttachmentRef(
          2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

  std::vector<VkAttachmentReference> colorAttachmentsRefs{colorAttachmentRef};
  VkSubpassDescription subpass = RenderPassUtils::createSubpassDescription(
      VK_PIPELINE_BIND_POINT_GRAPHICS, colorAttachmentsRefs,
      &depthAttachmentRef, &colorAttachmentResolveRef);
  VkSubpassDependency dependency = RenderPassUtils::createSubpassDependency(
      VK_SUBPASS_EXTERNAL, 0,
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
          VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
      0,
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
          VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
      VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
          VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT);

  std::vector<VkAttachmentDescription> attachments = {
      colorAttachment, depthAttachment, colorAttachmentResolve};
  renderPass = new RenderPass(device, attachments, subpass, dependency);

  swapchain->createFramebuffers(device, renderPass);

  VkDescriptorSetLayoutBinding cameraLayoutBinding =
      DescriptorUtils::createDescriptorSetBinding(
          0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, nullptr,
          VK_SHADER_STAGE_VERTEX_BIT);
  VkDescriptorSetLayoutBinding sceneLayoutBinding =
      DescriptorUtils::createDescriptorSetBinding(
          1, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, nullptr,
          VK_SHADER_STAGE_FRAGMENT_BIT);
  std::vector<VkDescriptorSetLayoutBinding> uboBindings = {cameraLayoutBinding,
                                                           sceneLayoutBinding};

  VkDescriptorSetLayoutBinding textureSamplerLayoutBinding =
      DescriptorUtils::createDescriptorSetBinding(
          0,
          sceneDependancies->textures.size() * 3,  // TODO
          VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, nullptr,
          VK_SHADER_STAGE_FRAGMENT_BIT);
  std::vector<VkDescriptorSetLayoutBinding> textureBindings = {
      textureSamplerLayoutBinding};

  graphicsDescriptorSetLayout = new Descriptor(device, uboBindings);
  graphicsMaterialDescriptorSetLayout = new Descriptor(device, textureBindings);
  std::vector<VkDescriptorSetLayout> graphicsDescriptorSetLayouts = {
      graphicsDescriptorSetLayout->getLayout(),
      graphicsMaterialDescriptorSetLayout->getLayout()};

  // computeDescriptorSetLayout = new Descriptor(device, uboBindings);
  // std::vector<VkDescriptorSetLayout> computeDescriptorSetLayouts = {
  //     computeDescriptorSetLayout->getLayout()};

  graphicsPipelines.resize(sceneDependancies->shaderPairs.size());
  for (size_t i = 0; i < graphicsPipelines.size(); i++) {
    graphicsPipelines[i] =
        new GraphicsPipeline(device, sceneDependancies->shaderPairs[i],
                             graphicsDescriptorSetLayouts, renderPass);
  }

  // computePipelines.resize(sceneDependancies->computeShaders.size());
  // for (size_t i = 0; i < computePipelines.size(); i++) {
  //   computePipelines[i] =
  //       new ComputePipeline(device, sceneDependancies->computeShaders[i],
  //                           computeDescriptorSetLayouts);
  // }

  modelBufferManager = new ModelBufferManager(device);

  textureManager->loadTextures(device, graphicsCommandPool,
                               sceneDependancies->textures);
  modelBufferManager->generateDescriptorSets(
      device, graphicsDescriptorSetLayouts, textureManager->getTextures());
  createSyncObjects();
}

void DefaultRenderer::cleanup() {
  for (size_t i = 0; i < RenderConst::MAX_FRAMES_IN_FLIGHT; i++) {
    vkDestroySemaphore(device->getLogicalDevice(), renderFinishedSemaphores[i],
                       nullptr);
    vkDestroySemaphore(device->getLogicalDevice(), imageAvailableSemaphores[i],
                       nullptr);
    vkDestroyFence(device->getLogicalDevice(), inFlightFences[i], nullptr);
  }
  renderPass->cleanup(device);
  delete renderPass;
  modelBufferManager->cleanup(device);
  delete modelBufferManager;
  for (GraphicsPipeline* pipeline : graphicsPipelines) {
    pipeline->cleanup(device);
    delete pipeline;
  }
  for (ComputePipeline* pipeline : computePipelines) {
    pipeline->cleanup(device);
    delete pipeline;
  }
  graphicsMaterialDescriptorSetLayout->cleanup(device);
  delete graphicsMaterialDescriptorSetLayout;
  graphicsDescriptorSetLayout->cleanup(device);
  delete graphicsDescriptorSetLayout;
  BaseRenderer::cleanup();
}

void DefaultRenderer::createSyncObjects() {
  imageAvailableSemaphores.resize(RenderConst::MAX_FRAMES_IN_FLIGHT);
  renderFinishedSemaphores.resize(RenderConst::MAX_FRAMES_IN_FLIGHT);
  inFlightFences.resize(RenderConst::MAX_FRAMES_IN_FLIGHT);

  VkSemaphoreCreateInfo semaphoreInfo{};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fenceInfo{};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  for (size_t i = 0; i < RenderConst::MAX_FRAMES_IN_FLIGHT; i++) {
    if (vkCreateSemaphore(device->getLogicalDevice(), &semaphoreInfo, nullptr,
                          &imageAvailableSemaphores[i]) != VK_SUCCESS ||
        vkCreateSemaphore(device->getLogicalDevice(), &semaphoreInfo, nullptr,
                          &renderFinishedSemaphores[i]) != VK_SUCCESS ||
        vkCreateFence(device->getLogicalDevice(), &fenceInfo, nullptr,
                      &inFlightFences[i]) != VK_SUCCESS) {
      throw std::runtime_error(
          "failed to create synchronization objects for a frame!");
    }
  }
}

void DefaultRenderer::draw(const bool& framebufferResized,
                           RenderSceneData& sceneData) {
  BaseRenderer::draw(framebufferResized, sceneData);
  vkWaitForFences(device->getLogicalDevice(), 1, &inFlightFences[frameIndex],
                  VK_TRUE, UINT64_MAX);

  uint32_t imageIndex;
  VkResult result = vkAcquireNextImageKHR(
      device->getLogicalDevice(), swapchain->getSwapchain(), UINT64_MAX,
      imageAvailableSemaphores[frameIndex], VK_NULL_HANDLE, &imageIndex);

  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    swapchain->reCreateSwapchain(device, renderPass);
    return;
  } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    throw std::runtime_error("failed to acquire swap chain image!");
  }

  modelBufferManager->updateUniformBuffer(device, frameIndex, sceneData);

  vkResetFences(device->getLogicalDevice(), 1, &inFlightFences[frameIndex]);
  VkCommandBuffer currentCommandBuffer =
      graphicsCommandPool->getCommandBuffer(frameIndex);

  vkResetCommandBuffer(currentCommandBuffer, 0);

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

  if (vkBeginCommandBuffer(currentCommandBuffer, &beginInfo) != VK_SUCCESS) {
    throw std::runtime_error("failed to begin recording command buffer!");
  }

  std::vector<VkClearValue> clearValues({VkClearValue(), VkClearValue()});
  clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
  clearValues[1].depthStencil = {1.0f, 0};

  renderPass->startRenderPass(
      currentCommandBuffer, swapchain->getFramebuffer(imageIndex),
      swapchain->getSwapchainExtent(), clearValues, VK_SUBPASS_CONTENTS_INLINE);

  swapchain->setViewport(currentCommandBuffer);
  swapchain->setScissor(currentCommandBuffer);

  meshManager->bindBuffers(currentCommandBuffer);
  int modelID = 0;
  graphicsPipelines[0]->bindPipeline(currentCommandBuffer,
                                     VK_PIPELINE_BIND_POINT_GRAPHICS);
  graphicsPipelines[0]->bindDescriptorSets(
      currentCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, 0, 1,
      modelBufferManager->getGlobalDescriptorSet(frameIndex), 0, nullptr);
  graphicsPipelines[0]->bindDescriptorSets(
      currentCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, 1, 1,
      modelBufferManager->getTextureDescriptorSet(frameIndex), 0, nullptr);
  meshManager->drawIndexedIndirect(currentCommandBuffer);

  renderPass->endRenderPass(currentCommandBuffer);

  if (vkEndCommandBuffer(currentCommandBuffer) != VK_SUCCESS) {
    throw std::runtime_error("failed to record command buffer!");
  }

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[frameIndex]};
  VkPipelineStageFlags waitStages[] = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = waitSemaphores;
  submitInfo.pWaitDstStageMask = waitStages;

  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &currentCommandBuffer;

  VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[frameIndex]};
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = signalSemaphores;

  if (vkQueueSubmit(device->getGraphicsQueue(), 1, &submitInfo,
                    inFlightFences[frameIndex]) != VK_SUCCESS) {
    throw std::runtime_error("failed to submit draw command buffer!");
  }

  VkPresentInfoKHR presentInfo{};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = signalSemaphores;
  presentInfo.swapchainCount = 1;
  VkSwapchainKHR swapChains[] = {swapchain->getSwapchain()};
  presentInfo.pSwapchains = swapChains;
  presentInfo.pImageIndices = &imageIndex;

  result = vkQueuePresentKHR(device->getPresentQueue(), &presentInfo);

  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR /*||
      framebufferResized*/) {
    swapchain->reCreateSwapchain(device, renderPass);
  } else if (result != VK_SUCCESS) {
    throw std::runtime_error("failed to present swap chain image!");
  }

  frameIndex = (frameIndex + 1) % RenderConst::MAX_FRAMES_IN_FLIGHT;
}