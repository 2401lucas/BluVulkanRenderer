#version 450

layout(location = 0) in vec2 fragTexCord;

layout (binding = 0) uniform UBO 
{
	float aaType;
} ubo;

layout(binding = 1) uniform sampler2D screenTexture;

layout (location = 0) out vec4 outColor;

#include "includes/tonemapping.glsl"
#include "includes/fxaa.glsl"


const int ANTI_ALIASING_OFF = 0;
const int ANTI_ALIASING_FXAA = 1;
const int ANTI_ALIASING_TAA = 2;

void main(){
	vec3 rgb =  texture(screenTexture, fragTexCord).rgb;
	//Early
	
	vec3 color = vec3(0,0,0);
	int aaType = int(ubo.aaType);
	if(aaType == ANTI_ALIASING_OFF) {
		color = ApplyTonemap(rgb);
	} else if(aaType == ANTI_ALIASING_FXAA) { 
		color = ApplyFXAA(ApplyFilter(rgb)); 
	} else if(aaType == ANTI_ALIASING_TAA) {
		//color = ApplyTAA(ApplyFilter(rgb));
	}

	//Final
	outColor = vec4(color, 1.0);
}