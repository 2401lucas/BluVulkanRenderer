#pragma once

#include "Pipeline.h"
#include "../Descriptors/Descriptor.h"
#include "../RenderPass/RenderPass.h"

class GraphicsPipeline : public Pipeline {
public:
	GraphicsPipeline(Device*, std::vector<ShaderInfo>, const std::vector<VkDescriptorSetLayout>& descriptorLayouts, RenderPass*);
};