#pragma once
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>

struct RenderLightData {
public:
	glm::vec4 lightColor;
	glm::vec3 lightPosition;
	glm::vec3 lightRotation;
	float constant;
	float linear;
	float quad;
	float innerCutoff;
	float outerCutoff;
	int lightType;
};