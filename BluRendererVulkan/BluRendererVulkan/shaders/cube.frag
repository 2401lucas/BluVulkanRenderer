#version 450

layout (location = 0) in vec3 inWorldPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inUV;

layout(set = 1, binding = 0) uniform sampler2D texArray[10];

layout (location = 0) out vec4 outColor;

void main() {
  outColor = vec4(texture(texArray[0], inUV.xy));
}