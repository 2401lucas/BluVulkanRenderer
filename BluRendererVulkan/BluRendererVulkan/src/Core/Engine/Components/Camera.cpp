#include "Camera.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace blu::game::components {
Camera::Camera(Transform* transform, float aspect_ratio, float fov,
               float z_near, float z_far) {
  transform_ = transform;
  aspect_ratio_ = aspect_ratio;
  fov_ = fov;
  z_near_ = z_near;
  z_far_ = z_far;
  CalculatePerspectiveMatrix();
}

void Camera::Update() {
  if (perspective_mat_updated_) {
    CalculatePerspectiveMatrix();
  }
}

void Camera::CalculatePerspectiveMatrix() {
  perspective_mat_ =
      glm::perspective(glm::radians(fov_), aspect_ratio_, z_near_, z_far_);
  perspective_mat_[1][1] *= -1;
  perspective_mat_updated_ = false;
}
}  // namespace blu::core::components