#version 450

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec4 clipSpaceFragPos;

layout (binding = 0) uniform UBO 
{
	mat4 proj;
	mat4 inverseProj;
	mat4 inverseView;
	vec2 noiseScale;
	vec2 fovScale;
	float zNear;
	float zFar;
	float kernelSize;
	float radius;
	float aaType;
    float aoType;
    bool enableBloom;
} ubo;

layout (location = 0) out vec4 outColor;

layout (set = 1, binding = 0) uniform AOSampling 
{
    vec4 mSSAOKernel[12];
} aoSampling;

layout(set = 1, binding = 1) uniform sampler2D randomTexture;
layout(set = 1, binding = 2) uniform sampler2D depthTexture;

#include "../includes/PostProcessing/ssao.glsl"

void main(){
    outColor = vec4(ApplySSAO(), 0.0);
    //outColor = vec4(texture(screenTexture, fragTexCoord).rgb, 0.0);
}