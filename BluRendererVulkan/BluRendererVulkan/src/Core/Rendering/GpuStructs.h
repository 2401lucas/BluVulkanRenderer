#ifndef GPUSTRUCTS_H
#define GPUSTRUCTS_H

struct GPUModelData {
  glm::vec4 sphere_bounding_box;
  uint32_t model_id;

  uint32_t pad1;
  uint32_t pad2;
  uint32_t pad3;
};
static_assert(sizeof(GPUModelData) == 32);

struct GPUModelIndices {
  // Model
  int vert_offset;
  uint32_t ind_count;
  uint32_t ind_offset;
  // Texture
  uint32_t material_type;
  int base_tex_id;
  int normal_tex_id;
  int emission_tex_id;
  int metalness_tex_id;
  int diffuse_roughness_id;
  int ambient_occlusion_id;
};
static_assert(sizeof(GPUModelIndices) == 40);

struct GPUModelInfo {
  int model_id;
  // Used for Culling
  glm::vec4 model_bounding_box;
};

#endif