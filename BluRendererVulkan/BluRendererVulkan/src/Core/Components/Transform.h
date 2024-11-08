#ifndef TRANSFORM_H
#define TRANSFORM_H

#include <glm/matrix.hpp>
#include <glm/vec3.hpp>
#include <glm/trigonometric.hpp>

namespace blu::core::components {
class Transform {
 public:
  Transform(glm::vec3 position, glm::vec3 rotation, glm::vec3 scale);

  glm::vec3 GetPosition() const { return position_; }
  void SetPosition(glm::vec3 new_value) {
    position_ = new_value;
    transform_mat_updated_ = true;
  }
  void AddToPosition(glm::vec3 new_value) {
    position_ += new_value;
    transform_mat_updated_ = true;
  }

  glm::vec3 GetRotation() const { return rotation_; }
  void SetRotation(glm::vec3 new_value) {
    rotation_ = new_value;
    transform_mat_updated_ = true;
  }
  void AddToRotation(glm::vec3 new_value) {
    rotation_ += new_value;
    transform_mat_updated_ = true;
  }
  glm::vec3 GetScale() const { return scale_; }
  void SetScale(glm::vec3 new_value) {
    scale_ = new_value;
    transform_mat_updated_ = true;
  }
  void AddToScale(glm::vec3 new_value) {
    scale_ += new_value;
    transform_mat_updated_ = true;
  }

  glm::vec3 Front() const {
    glm::vec3 cam_front;
    cam_front.x =
        -cos(glm::radians(rotation_.x)) * sin(glm::radians(rotation_.y));
    cam_front.y = sin(glm::radians(rotation_.x));
    cam_front.z =
        cos(glm::radians(rotation_.x)) * cos(glm::radians(rotation_.y));
    cam_front = glm::normalize(cam_front);
    return cam_front;
  }

  glm::mat4 GetTransformMat() {
    if (transform_mat_updated_) CalculateTransformMat();
    return transform_mat_;
  }

 private:
  void CalculateTransformMat();

  bool transform_mat_updated_ = false;
  glm::mat4 transform_mat_;

  glm::vec3 position_;
  glm::vec3 rotation_;
  glm::vec3 scale_;
};
}  // namespace blu::core::components
#endif