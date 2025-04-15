#ifndef RENDER_DATA_H
#define RENDER_DATA_H

#include <EASTL/vector.h>

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

struct ModelData {
  glm::vec4 sphere_bounding_box;
  uint32_t model_id;

  uint32_t pad1;
  uint32_t pad2;
  uint32_t pad3;
};

struct SceneInfo {
  glm::vec4 planes[6];
  eastl::vector<ModelData> model_data;
};

struct RenderData {
  // Matrix[0] - V * P
  // Matrix[1] - Camera View
  // Matrix[2] - Camera Perspective
  // Matrix[3...] - Model Position
  eastl::vector<glm::mat4> matrices;
  SceneInfo scene;
};

#endif