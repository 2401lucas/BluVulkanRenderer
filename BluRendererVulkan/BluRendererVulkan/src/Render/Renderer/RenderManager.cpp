#include "../Renderer/RenderManager.h"
#include "../RenderPass/RenderPassUtils.h"
#include "../Descriptors/DescriptorUtils.h"
#include "../Descriptors/Types/UBO/UBO.h"
#include "../src/Engine/Scene/SceneUtils.h"

RenderManager::RenderManager(GLFWwindow* window, const VkApplicationInfo& appInfo, DeviceSettings deviceSettings)
{
    auto buildDependencies = SceneUtils::getBuildDependencies();
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

    VkDescriptorSetLayoutBinding textureSamplerLayoutBinding = DescriptorUtils::createDescriptorSetBinding(0, buildDependencies.textures.size() * 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, nullptr, VK_SHADER_STAGE_FRAGMENT_BIT);
    //VkDescriptorSetLayoutBinding materialLayoutBinding = DescriptorUtils::createDescriptorSetBinding(1, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, nullptr, VK_SHADER_STAGE_FRAGMENT_BIT);
    std::vector<VkDescriptorSetLayoutBinding> materialBindings = { textureSamplerLayoutBinding};

    graphicsDescriptorSetLayout = new Descriptor(device, bindings);
    graphicsMaterialDescriptorSetLayout = new Descriptor(device, materialBindings);
    std::vector<VkDescriptorSetLayout> descriptorSetLayouts { graphicsDescriptorSetLayout->getLayout(), graphicsMaterialDescriptorSetLayout->getLayout() };
    standardGraphicsPipeline = new GraphicsPipeline(device, { buildDependencies.shaders[0], buildDependencies.shaders[1] }, descriptorSetLayouts, renderPass);
    wireframeGraphicsPipeline = new GraphicsPipeline(device, { buildDependencies.shaders[2], buildDependencies.shaders[3] }, descriptorSetLayouts, renderPass);
    graphicsCommandPool = new CommandPool(device, device->findQueueFamilies().graphicsFamily.value(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

    modelManager = new ModelManager(device);
    modelManager->loadTextures(device, graphicsCommandPool, buildDependencies.textures);
    descriptorManager = new DescriptorSetManager(device, descriptorSetLayouts, modelManager);
    graphicsCommandPool->createCommandBuffers(device);
    
    //TODO Update to allow transformations & rotations via input, requiring camera to exist not in the render manager. Also take everything non render manager related OUT include Scenes, model loading & more
    camera = new Camera(glm::fvec3(2.0f, 2.0f, 2.0f), glm::fvec3(0.0f, 0.0f, 0.0f), glm::radians(45.0f), swapchain->getExtentRatio(), 0.1f, 10.0f);

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

    delete camera;
    descriptorManager->cleanup(device);
    delete descriptorManager;
    modelManager->cleanup(device);
    delete modelManager;
    graphicsCommandPool->cleanup(device);
    delete graphicsCommandPool;
    standardGraphicsPipeline->cleanup(device);
    delete standardGraphicsPipeline;
    wireframeGraphicsPipeline->cleanup(device);
    delete wireframeGraphicsPipeline;
    graphicsMaterialDescriptorSetLayout->cleanup(device);
    delete graphicsMaterialDescriptorSetLayout;
    graphicsDescriptorSetLayout->cleanup(device);
    delete graphicsDescriptorSetLayout;
    swapchain->cleanup(device);
    delete swapchain;
    renderPass->cleaup(device);
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

bool temp = false;
//TODO: Load static models once
void RenderManager::drawFrame(const bool& framebufferResized, const SceneInfo* sceneInfo)
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

    //TODO: Support multiple cameras
    camera->updateCamera(sceneInfo->cameras[0]);
    //TODO: Load models only when the model list is updated
    if (!temp)
    {
        modelManager->loadModels(device, graphicsCommandPool, sceneInfo->dynamicModels);
        temp = true;
    }
    
    modelManager->updateUniformBuffer(device, camera, sceneInfo, 0, frameIndex);

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

    auto modelCount = modelManager->getModelCount();

    wireframeGraphicsPipeline->bindPipeline(currentCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS);
    wireframeGraphicsPipeline->bindDescriptorSets(currentCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, 0, 1, descriptorManager->getGlobalDescriptorSet(frameIndex), 0, nullptr);
    standardGraphicsPipeline->bindDescriptorSets(currentCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, 1, 1, descriptorManager->getMaterialDescriptorSet(frameIndex), 0, nullptr);

    for (int i = 0; i < modelCount; i++)
    {
        modelManager->bindBuffers(currentCommandBuffer, i);
        modelManager->updatePushConstants(currentCommandBuffer, standardGraphicsPipeline->getPipelineLayout(), i);

        modelManager->drawIndexed(currentCommandBuffer, i);
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