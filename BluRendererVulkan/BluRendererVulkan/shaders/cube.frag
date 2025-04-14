#version 450

#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_buffer_reference2 : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : enable

layout (location = 0) in vec3 inWorldPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inUV;
layout (location = 3) flat in int inInstanceIndex;

layout(set = 1, binding = 0) uniform sampler2D textures[10];

#include "includes/BufferInfo.glsl"
#include "includes/RenderModelData.glsl"
#include "includes/SceneModelData.glsl"


layout(buffer_reference, std430) buffer ModelInfoBuffer {
  RenderModelData info[];
};

layout(buffer_reference, std430) buffer ModelIndex {
  SceneModelData data[];
};

layout (set = 0, binding = 0) buffer BufferAddresses{
  BufferInfo bufferPointers[];
} bufferAddresses;

layout (location = 0) out vec4 outColor;

void main() {
  ModelInfoBuffer modelInfo = ModelInfoBuffer(bufferAddresses.bufferPointers[1].address);
  ModelIndex indexInfo = ModelIndex(bufferAddresses.bufferPointers[2].address);

  outColor = vec4(texture(
		textures[modelInfo.info[indexInfo.data[inInstanceIndex].id].base_tex_id], inUV.xy));
}