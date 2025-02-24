#ifndef RENDER_DATA_H
#define RENDER_DATA_H

#include <EASTL/vector.h>

struct RenderData {
  // Matrix[0] - Camera View
  // Matrix[1] - Camera Perspective
  // Matrix[2...] - Model Position
  eastl::vector<glm::mat4> matrices;
};

#endif