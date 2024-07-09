#version 450

layout (location = 0) in vec3 inPos;

layout (binding = 0) uniform UBO
{
	mat4 depthMVP[4];
} ubo;

layout (set = 0, binding = 1) uniform mUBO 
{
	mat4 models[16];
} mUbo;

layout (push_constant) uniform PushConstants {
	int depthMVPIndex;
	int transformIndex;
} pushConstants;

layout (set = 1, binding = 0) uniform UBONode {
	mat4 matrix;
} uboNode;

out gl_PerVertex
{
    vec4 gl_Position;
};

void main()
{
	vec4 locPos = mUbo.models[pushConstants.transformIndex] * vec4(inPos, 1.0);
	//vec3 pos = locPos.xyz / locPos.w;
	gl_Position =  ubo.depthMVP[pushConstants.depthMVPIndex] * locPos;
}