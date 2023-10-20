#pragma once

#include "../src/Pipeline/Pipeline.h"
#include "../Descriptors/Descriptor.h"
#include "../RenderPass/RenderPass.h"

class GraphicsPipeline : public Pipeline {
public:
	GraphicsPipeline(Device*, std::vector<ShaderInfo>, Descriptor*, RenderPass*);
};