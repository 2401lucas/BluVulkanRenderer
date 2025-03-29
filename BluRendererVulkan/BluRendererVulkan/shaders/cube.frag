#version 450

layout (location = 0) in vec3 inWorldPos;
layout (location = 1) in vec3 inUV;

layout (location = 0) out vec4 outColor;

void main() {
  outColor = vec4(1,0,1,1);
}