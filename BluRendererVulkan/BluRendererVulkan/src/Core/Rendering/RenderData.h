#ifndef RENDER_DATA_H
#define RENDER_DATA_H

#include <EASTL/vector.h>

struct RenderData {
  // Matrix[0] - V * P
  // Matrix[1] - Camera View
  // Matrix[2] - Camera Perspective
  // Matrix[3...] - Model Position
  eastl::vector<glm::mat4> matrices;
  eastl::vector<uint32_t> model_ids;
};

#endif