#pragma once
#include <vulkan/vulkan_core.h>
#include <vector>

class RenderPassUtils {
public:
	RenderPassUtils() = delete;

	static VkAttachmentDescription createAttachmentDescription(const VkFormat& format, const VkSampleCountFlagBits& samples, const VkAttachmentLoadOp& loadOp, const VkAttachmentStoreOp& storeOp, const VkAttachmentLoadOp& stencilLoadOp, const VkAttachmentStoreOp& stencilStoreOp, const VkImageLayout& initLayout, const VkImageLayout& finalLayout);
	static VkAttachmentReference createAttachmentRef(const uint32_t& attachment, const VkImageLayout& layout);

	static VkSubpassDescription createSubpassDescription(const VkPipelineBindPoint& pipelineBindPoint, const std::vector<VkAttachmentReference>& colorRefs, const VkAttachmentReference* depthRef, const VkAttachmentReference* resolveRefs);
	static VkSubpassDependency createSubpassDependency(const uint32_t& srcSubpass, const uint32_t& dstSubpass, const VkPipelineStageFlags& scrStageMask, const VkAccessFlags& srcAccessMask, const VkPipelineStageFlags& dstStageMask, const VkAccessFlags& dstAccessMask);
};