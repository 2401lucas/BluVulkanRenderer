#version 450

layout(location = 0) in vec2 fragTexCord;

layout (binding = 0) uniform UBO 
{
	bool useFXAA;
} ubo;

layout(binding = 1) uniform sampler2D screenTexture;

layout (location = 0) out vec4 outColor;

#include "tonemapping.frag"
#include "fxaa.frag"

void main(){
	//Early



	//Final
	outColor = vec4(applyFXAA(), 1.0);
}