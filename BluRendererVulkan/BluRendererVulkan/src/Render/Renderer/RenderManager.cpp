#include "../Renderer/RenderManager.h"
#include "../RenderPass/RenderPassUtils.h"
#include "../Descriptors/DescriptorUtils.h"
#include "../Descriptors/Types/UBO/UBO.h"

RenderManager::RenderManager(GLFWwindow* window, const VkApplicationInfo& appInfo, DeviceSettings deviceSettings, const SceneDependancies* sceneDependancies)
{
    vkInstance = new VulkanInstance(appInfo);
    device = new Device(window, vkInstance, deviceSettings);
    swapchain = new Swapchain(device);

    VkAttachmentDescription colorAttachment = RenderPassUtils::createAttachmentDescription(swapchain->getSwapchainFormat(), device->getMipSampleCount(), VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    VkAttachmentDescription depthAttachment = RenderPassUtils::createAttachmentDescription(device->findSupportedFormat({ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT }, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT), 
        device->getMipSampleCount(), VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    VkAttachmentDescription colorAttachmentResolve = RenderPassUtils::createAttachmentDescription(swapchain->getSwapchainFormat(), VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_STORE, VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    VkAttachmentReference colorAttachmentRef = RenderPassUtils::createAttachmentRef(0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    VkAttachmentReference depthAttachmentRef = RenderPassUtils::createAttachmentRef(1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    VkAttachmentReference colorAttachmentResolveRef = RenderPassUtils::createAttachmentRef(2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    std::vector<VkAttachmentReference> colorAttachmentsRefs { colorAttachmentRef };
    VkSubpassDescription subpass = RenderPassUtils::createSubpassDescription(VK_PIPELINE_BIND_POINT_GRAPHICS, colorAttachmentsRefs, &depthAttachmentRef, &colorAttachmentResolveRef);
    VkSubpassDependency dependency = RenderPassUtils::createSubpassDependency(VK_SUBPASS_EXTERNAL, 0, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, 0, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT);

    std::vector<VkAttachmentDescription> attachments = { colorAttachment, depthAttachment, colorAttachmentResolve };
    renderPass = new RenderPass(device, attachments, subpass, dependency);

    swapchain->createFramebuffers(device, renderPass);

    VkDescriptorSetLayoutBinding cameraLayoutBinding = DescriptorUtils::createDescriptorSetBinding(0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, nullptr, VK_SHADER_STAGE_VERTEX_BIT);
    VkDescriptorSetLayoutBinding sceneLayoutBinding = DescriptorUtils::createDescriptorSetBinding(1, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, nullptr, VK_SHADER_STAGE_FRAGMENT_BIT);
    std::vector<VkDescriptorSetLayoutBinding> bindings = { cameraLayoutBinding, sceneLayoutBinding};

    VkDescriptorSetLayoutBinding textureSamplerLayoutBinding = DescriptorUtils::createDescriptorSetBinding(0, sceneDependancies->textures.size() * 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, nullptr, VK_SHADER_STAGE_FRAGMENT_BIT);
    //VkDescriptorSetLayoutBinding materialLayoutBinding = DescriptorUtils::createDescriptorSetBinding(1, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, nullptr, VK_SHADER_STAGE_FRAGMENT_BIT);
    std::vector<VkDescriptorSetLayoutBinding> materialBindings = { textureSamplerLayoutBinding };

    graphicsDescriptorSetLayout = new Descriptor(device, bindings);
    graphicsMaterialDescriptorSetLayout = new Descriptor(device, materialBindings);
    std::vector<VkDescriptorSetLayout> descriptorSetLayouts = { graphicsDescriptorSetLayout->getLayout(), graphicsMaterialDescriptorSetLayout->getLayout() };
    
    graphicsPipelines.resize(sceneDependancies->shaders.size() / 2);
    for (size_t i = 0; i < graphicsPipelines.size(); i++) {
        graphicsPipelines[i] = new GraphicsPipeline(device, { sceneDependancies->shaders[i * 2], sceneDependancies->shaders[i * 2 + 1] }, descriptorSetLayouts, renderPass);
    }

    graphicsCommandPool = new CommandPool(device, device->findQueueFamilies().graphicsFamily.value(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    graphicsCommandPool->createCommandBuffers(device);

    modelBufferManager = new ModelBufferManager(device);

    textureManager = new TextureManager();
    textureManager->loadTextures(device, graphicsCommandPool, sceneDependancies->textures);

    descriptorManager = new DescriptorSetManager(device, descriptorSetLayouts, modelBufferManager, textureManager);

    createSyncObjects();

    frameIndex = 0;
}

void RenderManager::cleanup()
{
    for (size_t i = 0; i < RenderConst::MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(device->getLogicalDevice(), renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(device->getLogicalDevice(), imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(device->getLogicalDevice(), inFlightFences[i], nullptr);
    }

    descriptorManager->cleanup(device);
    delete descriptorManager;
    modelBufferManager->cleanup(device);
    delete modelBufferManager;
    graphicsCommandPool->cleanup(device);
    delete graphicsCommandPool;
    for(GraphicsPipeline* pipeline : graphicsPipelines) {
        pipeline->cleanup(device);
        delete pipeline;
    }
    graphicsMaterialDescriptorSetLayout->cleanup(device);
    delete graphicsMaterialDescriptorSetLayout;
    graphicsDescriptorSetLayout->cleanup(device);
    delete graphicsDescriptorSetLayout;
    swapchain->cleanup(device);
    delete swapchain;
    renderPass->cleanup(device);
    delete renderPass;
    device->cleanup(vkInstance);
    delete device;
    vkInstance->cleanup();
    delete vkInstance;
}

void RenderManager::createSyncObjects() {
    imageAvailableSemaphores.resize(RenderConst::MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(RenderConst::MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(RenderConst::MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < RenderConst::MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(device->getLogicalDevice(), &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(device->getLogicalDevice(), &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(device->getLogicalDevice(), &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create synchronization objects for a frame!");
        }
    }
}

//Sum of Vertex/Index size should be saved to prealloc buffers for GPU? Should buffers be dynamically size or have a max size that it cannot exceed?
//Should there be one buffer per index/vertex, or one for both
//Update the UBO with model info (Should the model data be contiguous(Would that impact the effectiveness of ECS and if so how do other(Unity) systems handle the data transportation to the renderer/How do they manage the pipelines to prevent the rebinding of pipelines))
//Descriptor sets/Graphics pipeline layout, how many should I have? How do we programmatically create pipelines that share the same GPL? Is there a benefit to multiple Descriptor sets? 
//Descriptor sets could be split based on static and dynamic models
//Register Models once, only updating the vertex buffer once
void RenderManager::drawFrame(const bool& framebufferResized, RenderSceneData& sceneData)
{
    vkWaitForFences(device->getLogicalDevice(), 1, &inFlightFences[frameIndex], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(device->getLogicalDevice(), swapchain->getSwapchain(), UINT64_MAX, imageAvailableSemaphores[frameIndex], VK_NULL_HANDLE, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        swapchain->reCreateSwapchain(device, renderPass);
        return;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }
    
    modelBufferManager->updateUniformBuffer(device, frameIndex, sceneData);

    vkResetFences(device->getLogicalDevice(), 1, &inFlightFences[frameIndex]);
    VkCommandBuffer currentCommandBuffer = graphicsCommandPool->getCommandBuffer(frameIndex);

    vkResetCommandBuffer(currentCommandBuffer, 0);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(currentCommandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }

    std::vector<VkClearValue> clearValues({VkClearValue(), VkClearValue()});
    clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
    clearValues[1].depthStencil = { 1.0f, 0 };

    renderPass->startRenderPass(currentCommandBuffer, swapchain->getFramebuffer(imageIndex), swapchain->getSwapchainExtent(), clearValues, VK_SUBPASS_CONTENTS_INLINE);

    swapchain->setViewport(currentCommandBuffer);
    swapchain->setScissor(currentCommandBuffer);

    modelBufferManager->bindBuffers(currentCommandBuffer);
    int modelID = 0;
    for (auto& piplineIndex : sceneData.modelData)
    {
        graphicsPipelines[piplineIndex.first]->bindPipeline(currentCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS);
        graphicsPipelines[piplineIndex.first]->bindDescriptorSets(currentCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, 0, 1, descriptorManager->getGlobalDescriptorSet(frameIndex), 0, nullptr);
        graphicsPipelines[piplineIndex.first]->bindDescriptorSets(currentCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, 1, 1, descriptorManager->getMaterialDescriptorSet(frameIndex), 0, nullptr);
        for (int i = 0; i < piplineIndex.second.size(); i++)
        {
            modelBufferManager->updatePushConstants(currentCommandBuffer, graphicsPipelines[piplineIndex.first]->getPipelineLayout(), PushConstantData(modelID, piplineIndex.second[i].materialData));
            modelBufferManager->drawIndexed(currentCommandBuffer, piplineIndex.second[i].meshRenderData.indexCount, piplineIndex.second[i].meshRenderData.vertexMemChunk.offset, piplineIndex.second[i].meshRenderData.indexMemChunk.offset);
            modelID++;
        }
    }

    renderPass->endRenderPass(currentCommandBuffer);

    if (vkEndCommandBuffer(currentCommandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[frameIndex] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &currentCommandBuffer;

    VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[frameIndex] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(device->getGraphicsQueue(), 1, &submitInfo, inFlightFences[frameIndex]) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    presentInfo.swapchainCount = 1;
    VkSwapchainKHR swapChains[] = { swapchain->getSwapchain() };
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;

    result = vkQueuePresentKHR(device->getPresentQueue(), &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
        swapchain->reCreateSwapchain(device, renderPass);
    }
    else if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image!");
    }
    
    frameIndex = (frameIndex + 1) % RenderConst::MAX_FRAMES_IN_FLIGHT;
}

int RenderManager::getTextureIndex(TextureInfo textureInfo)
{
    return textureManager->getTextureIndex(textureInfo);
}

std::pair<MemoryChunk, MemoryChunk> RenderManager::registerMesh(RenderModelCreateData modelCreateInfo)
{
    return modelBufferManager->loadModelIntoBuffer(device, graphicsCommandPool, modelCreateInfo);
}