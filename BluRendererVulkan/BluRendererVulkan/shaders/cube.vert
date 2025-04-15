#version 450

#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_ARB_shader_draw_parameters : enable
#extension GL_EXT_buffer_reference2 : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : enable


#include "includes/BufferInfo.glsl"

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inUV;

layout(buffer_reference, std430) buffer MatrixBuffer {
  // [0] - Perspective * View
  // [1] - Perspective
  // [2] - View
  // [3+] - Model Pos
  mat4 matrices[];
};

layout (set = 0, binding = 0) buffer BufferAddresses{
  BufferInfo bufferPointers[];
} bufferAddresses;

layout (location = 0) out vec3 outPos;
layout (location = 1) out vec3 outNormal;
layout (location = 2) out vec3 outUV;
layout (location = 3) out int outInstanceIndex;

void main() {
  MatrixBuffer matBuf = MatrixBuffer(bufferAddresses.bufferPointers[0].address);
  outInstanceIndex = gl_InstanceIndex;

  outUV = inUV;
  outNormal = normalize(transpose(inverse(mat3(matBuf.matrices[3 + gl_InstanceIndex]))) * inNormal);

  vec4 locPos = matBuf.matrices[3 + gl_InstanceIndex] * vec4(inPos, 1.0f);
  outPos = locPos.xyz / locPos.w;
  gl_Position =  matBuf.matrices[0] * vec4(outPos, 1.0);
}