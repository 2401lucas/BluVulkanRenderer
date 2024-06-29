#version 450

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV;

layout (binding = 0) uniform UBO 
{
    mat4 models[16];
    mat4 projection;
} ubo;

layout (push_constant) uniform PushConstants {
	int materialIndex;
	int transformIndex;
} pushConstants;

layout (location = 0) out vec3 outUVW;

out gl_PerVertex 
{
	vec4 gl_Position;
};

void main() 
{
	outUVW = inPos;
	gl_Position = ubo.projection * ubo.models[0] * vec4(inPos.xyz, 1.0);
}
