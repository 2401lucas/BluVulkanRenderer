#version 450

#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_buffer_reference2 : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : enable

layout (location = 0) in vec3 inWorldPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inUV;
layout (location = 3) flat in int inInstanceIndex;

layout(set = 1, binding = 0) uniform sampler2D textures[10];

struct ModelInfo
{
	int vert_offset;
	uint ind_count;
	uint ind_offset;
	uint material_type;  
	int base_tex_id;
	int normal_tex_id;
	int emission_tex_id;
	int metalness_tex_id;
	int diffuse_roughness_id;
	int ambient_occlusion_id;
};

layout(buffer_reference, std430) buffer ModelInfoBuffer {
  ModelInfo info[];
};

layout(buffer_reference, std430) buffer ModelIndex {
  uint index[];
};

struct BufferInfo {
  uint64_t address;
  uint64_t offset;
  uint64_t size;
};

layout (set = 0, binding = 0) buffer BufferAddresses{
  BufferInfo bufferPointers[];
} bufferAddresses;

layout (location = 0) out vec4 outColor;

void main() {
  ModelInfoBuffer modelInfo = ModelInfoBuffer(bufferAddresses.bufferPointers[1].address);
  ModelIndex indexInfo = ModelIndex(bufferAddresses.bufferPointers[2].address);

  outColor = vec4(texture(
		textures[modelInfo.info[indexInfo.index[inInstanceIndex]].base_tex_id], inUV.xy));
}