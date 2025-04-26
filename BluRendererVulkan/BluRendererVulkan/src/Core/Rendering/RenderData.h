#ifndef RENDER_DATA_H
#define RENDER_DATA_H

#include <EASTL/vector.h>

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#include "GpuStructs.h"

struct RenderData {
  // Matrix[0] - V * P
  // Matrix[1] - Camera View
  // Matrix[2] - Camera Perspective
  // Matrix[3...] - Model Position
  eastl::vector<glm::mat4> matrices;
  glm::vec4 planes[6];
  eastl::vector<GPUModelData> model_data;
};

#endif