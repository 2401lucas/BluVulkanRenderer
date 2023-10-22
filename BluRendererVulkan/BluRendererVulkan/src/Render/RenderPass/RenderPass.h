#pragma once
#include "../Device/Device.h"

class RenderPass {
public: 
	RenderPass(Device* deviceInfo, std::vector<VkAttachmentDescription>& attachments, VkSubpassDescription& subpasses, VkSubpassDependency& dependencies);
	void cleaup(Device* deviceInfo);

	VkRenderPass& getRenderPass();
	void startRenderPass(const VkCommandBuffer& commandBuffer, const VkFramebuffer& frameBuffer, const VkExtent2D& extent, const std::vector<VkClearValue>& clearValues, const VkSubpassContents& commandSubpassContentsLayout);
	void endRenderPass(VkCommandBuffer& commandBuffer);
private:
	VkRenderPass renderPass;
};