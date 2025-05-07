#ifndef LIGHT_H
#define LIGHT_H

#include "Transform.h"

namespace blu::game::components {
struct Light {
  blu::game::components::Transform transform;
  // XYZ -> RGB
  // W -> Intensity
  glm::vec4 light_data;
  int light_type;
};
}  // namespace blu::game::components
#endif