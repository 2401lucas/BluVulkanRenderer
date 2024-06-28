#version 450

#include "../includes/PostProcessing/postProcessingFragGeneric.glsl"
#ifndef TONEMAP
    #define TONEMAP 1
#endif
#ifndef TONEMAP_EXPOSURE
	#define TONEMAP_EXPOSURE 4.5
#endif
#ifndef TONEMAP_GAMMA
	#define TONEMAP_GAMMA 2.2 
#endif

//Fast Filmic Tonemapping
vec3 Uncharted2Tonemap(vec3 x)
{
	float A = 0.15; //Shoulder Strength
	float B = 0.50; //Linear Strength
	float C = 0.10; //Linear Angle
	float D = 0.20; //Toe Strength
	float E = 0.02; //Toe Numerator
	float F = 0.30; //Toe Denominator
	return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}

void main() {
	// Tone mapping
	vec3 color = Uncharted2Tonemap(texture(screenTexture, fragTexCord).rgb * TONEMAP_EXPOSURE);
	color = color * (1.0f / Uncharted2Tonemap(vec3(11.2f)));
	// Gamma correction
	color = pow(color, vec3(1.0f / TONEMAP_GAMMA));
	outColor = vec4(color, 0.0);
}