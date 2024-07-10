layout(location = 0) in vec2 fragTexCord;

layout (binding = 0) uniform UBO 
{
    mat4 projMat;
    mat4 inverseProjMat;
    mat4 inverseViewMat;
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

layout(binding = 1) uniform sampler2D screenTexture;

layout (location = 0) out vec4 outColor;