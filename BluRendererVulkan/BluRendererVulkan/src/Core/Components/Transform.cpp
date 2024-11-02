#include "Transform.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace blu::core::components {
Transform::Transform(glm::vec3 position, glm::vec3 rotation, glm::vec3 scale) {
  position_ = position;
  rotation_ = rotation;
  scale_ = scale;
  CalculateTransformMat();
}

void Transform::CalculateTransformMat() {
  glm::mat4 rot_mat = glm::mat4(1.0f);
  rot_mat = glm::rotate(rot_mat, glm::radians(rotation_.x * -1.0f),
                        glm::vec3(1.0f, 0.0f, 0.0f));
  rot_mat = glm::rotate(rot_mat, glm::radians(rotation_.y),
                        glm::vec3(0.0f, 1.0f, 0.0f));
  rot_mat = glm::rotate(rot_mat, glm::radians(rotation_.z),
                        glm::vec3(0.0f, 0.0f, 1.0f));

  glm::mat4 trans_mat = glm::translate(glm::mat4(1.0f), position_);
  transform_mat_ = rot_mat * trans_mat;
  transform_mat_updated_ = false;
}
}  // namespace blu::core::components