#pragma once

#include "Pipeline.h"
#include "../Descriptors/Descriptor.h"
#include "../RenderPass/RenderPass.h"

class ComputePipeline : public Pipeline {
public:
	ComputePipeline(Device*, const ShaderInfo shader, const std::vector<VkDescriptorSetLayout>& descriptorLayouts);
};