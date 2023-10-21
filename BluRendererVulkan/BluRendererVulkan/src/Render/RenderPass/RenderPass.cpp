#include "RenderPass.h"
#include <stdexcept>

RenderPass::RenderPass(Device* deviceInfo, std::vector<VkAttachmentDescription>& attachments, VkSubpassDescription& subpasses, VkSubpassDependency& dependencies)
{
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpasses;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependencies;

    if (vkCreateRenderPass(deviceInfo->getLogicalDevice(), &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
        throw std::runtime_error("failed to create render pass!");
    }
}

void RenderPass::cleaup(Device* deviceInfo)
{
    vkDestroyRenderPass(deviceInfo->getLogicalDevice(), renderPass, nullptr);
}

VkRenderPass& RenderPass::getRenderPass()
{
    return renderPass;
}
