#pragma once

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

#include "../../Render/Components/Camera.hpp"

namespace core::engine::components {
class Camera {
 private:
  float fov;
  float znear, zfar;

 public:
  glm::vec3 position = glm::vec3(0);
  glm::vec3 rotation = glm::vec3(0);

  void updatePosition(glm::vec3 newPos);
  void updateRotation(glm::vec3 newRot);
};
}  // namespace core::engine::components