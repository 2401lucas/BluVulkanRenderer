#version 450

layout (binding = 2) uniform samplerCube samplerEnv;

layout (location = 0) in vec3 inUVW;

layout (location = 0) out vec4 outColor;

layout (set = 0, binding = 1) uniform UBOParams {
	vec4 lightDir;
	float exposure;
	float gamma;
	float prefilteredCubeMipLevels;
	float scaleIBLAmbient;
	float debugViewInputs;
	float debugViewEquation;
} uboParams;


#include "includes/tonemapping.glsl"

void main() 
{
	vec3 color = texture(samplerEnv, inUVW).rgb;

	// Tone mapping
	color = ApplyTonemap(color * uboParams.exposure);
	color = color * (1.0f / ApplyTonemap(vec3(11.2f)));	
	// Gamma correction
	color = pow(color, vec3(1.0f / uboParams.gamma));
	
	outColor = vec4(color, 1.0);
}