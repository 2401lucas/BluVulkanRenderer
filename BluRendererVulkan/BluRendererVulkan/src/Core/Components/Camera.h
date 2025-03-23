#ifndef CAMERA_H
#define CAMERA_H
#include <glm/matrix.hpp>

#include "Transform.h"

namespace blu::core::components {
class Camera {
 public:
  Camera(Transform* transform, float aspect_ratio, float fov, float z_near,
         float z_far);

  void Update();

  Transform* GetTransform() { return transform_; }

  float GetFov() const { return fov_; }
  void SetFov(float new_value) {
    fov_ = new_value;
    perspective_mat_updated_ = true;
  };
  float GetZNear() const { return z_near_; }
  void SetZNear(float new_value) {
    z_near_ = new_value;
    perspective_mat_updated_ = true;
  };
  float GetZFar() const { return z_far_; }
  void SetZFar(float new_value) {
    z_far_ = new_value;
    perspective_mat_updated_ = true;
  };

  void SetAspectRatio(float ratio) {
    aspect_ratio_ = ratio;
    perspective_mat_updated_ = true;
  }

  glm::mat4 GetPerspectiveMat() {
    if (perspective_mat_updated_) CalculatePerspectiveMatrix();
    return perspective_mat_;
  }

 private:
  void CalculatePerspectiveMatrix();

  Transform* transform_;

  bool perspective_mat_updated_ = false;
  glm::mat4 perspective_mat_;
  float aspect_ratio_;
  float fov_;
  float z_near_;
  float z_far_;
};
}  // namespace blu::core::components
#endif