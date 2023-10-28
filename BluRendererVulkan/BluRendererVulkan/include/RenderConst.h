#pragma once
#include <vulkan/vulkan_core.h>

class RenderConst {
public:
	static const uint32_t MAX_FRAMES_IN_FLIGHT = 2;
	static const uint32_t MAX_MODELS = 64;
	static const uint32_t MAX_TEXTURES = 64;
	static const uint32_t MAX_LIGHTS = 16;
};