layout(location = 0) in vec2 fragTexCord;

layout (binding = 0) uniform UBO 
{
	float aaType;
    float aoType;
    bool enableBloom;
} ubo;

layout(binding = 1) uniform sampler2D screenTexture;

layout (location = 0) out vec4 outColor;
