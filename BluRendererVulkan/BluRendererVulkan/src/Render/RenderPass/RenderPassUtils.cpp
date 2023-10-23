#include "RenderPassUtils.h"

VkAttachmentDescription RenderPassUtils::createAttachmentDescription(const VkFormat& format, const VkSampleCountFlagBits& samples, const VkAttachmentLoadOp& loadOp, const VkAttachmentStoreOp& storeOp, const VkAttachmentLoadOp& stencilLoadOp, const VkAttachmentStoreOp& stencilStoreOp, const VkImageLayout& initLayout, const VkImageLayout& finalLayout)
{
    VkAttachmentDescription attachmentDesc{};
    attachmentDesc.format = format;
    attachmentDesc.samples = samples;
    attachmentDesc.loadOp = loadOp;
    attachmentDesc.storeOp = storeOp;
    attachmentDesc.stencilLoadOp = stencilLoadOp;
    attachmentDesc.stencilStoreOp = stencilStoreOp;
    attachmentDesc.initialLayout = initLayout;
    attachmentDesc.finalLayout = finalLayout;

	return attachmentDesc;
}

VkAttachmentReference RenderPassUtils::createAttachmentRef(const uint32_t& attachment, const VkImageLayout& layout)
{
    VkAttachmentReference attachmentRef{};
    attachmentRef.attachment = attachment;
    attachmentRef.layout = layout;

    return attachmentRef;
}

VkSubpassDescription RenderPassUtils::createSubpassDescription(const VkPipelineBindPoint& pipelineBindPoint, const std::vector<VkAttachmentReference>* colorRefs, const VkAttachmentReference* depthRef, const VkAttachmentReference* resolveRefs)
{
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = (uint32_t)colorRefs->size();
    subpass.pColorAttachments = colorRefs->data();
    subpass.pDepthStencilAttachment = depthRef;
    subpass.pResolveAttachments = resolveRefs;

    return subpass;
}

VkSubpassDependency RenderPassUtils::createSubpassDependency(const uint32_t& srcSubpass, const uint32_t& dstSubpass, const VkPipelineStageFlags& srcStageMask, const VkAccessFlags& srcAccessMask, const VkPipelineStageFlags& dstStageMask, const VkAccessFlags& dstAccessMask)
{
    VkSubpassDependency dependency{};
    dependency.srcSubpass = srcSubpass;
    dependency.dstSubpass = dstSubpass;
    dependency.srcStageMask = srcStageMask;
    dependency.srcAccessMask = srcAccessMask;
    dependency.dstStageMask = dstStageMask;
    dependency.dstAccessMask = dstAccessMask;

    return dependency;
}
