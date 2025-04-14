#version 450

#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_ARB_shader_draw_parameters : enable
#extension GL_EXT_buffer_reference2 : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : enable

layout (location = 0) in vec3 inPos;

struct BufferInfo {
  uint64_t address;
  uint64_t offset;
  uint64_t size;
};

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

void main() {
  MatrixBuffer matBuf = MatrixBuffer(bufferAddresses.bufferPointers[0].address);
  vec4 locPos = matBuf.matrices[3 + gl_InstanceIndex] * vec4(inPos, 1.0f);
  gl_Position =  matBuf.matrices[0] * vec4(locPos.xyz / locPos.w, 1.0);
}