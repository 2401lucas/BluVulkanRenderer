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

void Camera::GetFrustumPlanes(const glm::mat4& viewProj,
                                   glm::vec4 planes[6]) {
  glm::mat4 m = viewProj;

  // Left
  planes[0] = glm::vec4(m[0][3] + m[0][0], m[1][3] + m[1][0], m[2][3] + m[2][0],
                        m[3][3] + m[3][0]);

  // Right
  planes[1] = glm::vec4(m[0][3] - m[0][0], m[1][3] - m[1][0], m[2][3] - m[2][0],
                        m[3][3] - m[3][0]);

  // Bottom
  planes[2] = glm::vec4(m[0][3] + m[0][1], m[1][3] + m[1][1], m[2][3] + m[2][1],
                        m[3][3] + m[3][1]);

  // Top
  planes[3] = glm::vec4(m[0][3] - m[0][1], m[1][3] - m[1][1], m[2][3] - m[2][1],
                        m[3][3] - m[3][1]);

  // Near
  planes[4] = glm::vec4(m[0][3] + m[0][2], m[1][3] + m[1][2], m[2][3] + m[2][2],
                        m[3][3] + m[3][2]);

  // Far
  planes[5] = glm::vec4(m[0][3] - m[0][2], m[1][3] - m[1][2], m[2][3] - m[2][2],
                        m[3][3] - m[3][2]);

  // Normalize
  for (int i = 0; i < 6; ++i) {
    float len = glm::length(glm::vec3(planes[i]));
    planes[i] /= len;
  }
}

void Camera::CalculatePerspectiveMatrix() {
  perspective_mat_ =
      glm::perspective(glm::radians(fov_), aspect_ratio_, z_near_, z_far_);
  perspective_mat_[1][1] *= -1;
  perspective_mat_updated_ = false;
}
}  // namespace blu::game::components