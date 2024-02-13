#pragma once
#include "../../../Render/Buffer/BufferAllocator.h"
#include "../../../Render/Math/MathUtils.h"
#include "BaseComponent.h"
#include <string>

struct MeshRenderer : BaseComponent {
  std::string path;
  uint32_t indexCount;
  BoundingBox boundingBox;
};