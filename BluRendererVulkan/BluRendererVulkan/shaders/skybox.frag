#version 450

layout (location = 0) in vec3 inUVW;

layout (binding = 2) uniform samplerCube samplerEnv;

layout (location = 0) out vec4 outColor;

void main() 
{
	vec3 color = texture(samplerEnv, inUVW).rgb;

	// Tone mapping
	// color = ApplyTonemap(color);
	
	outColor = vec4(color, 1.0);
}