#pragma once
#include "../Device/Device.h"

class RenderPass {
public: 
	RenderPass(Device* deviceInfo, std::vector<VkAttachmentDescription>& attachments, VkSubpassDescription& subpasses, VkSubpassDependency& dependencies);
	void cleaup(Device* deviceInfo);

	VkRenderPass& getRenderPass();
private:
	VkRenderPass renderPass;
};