#include "Engine.h"

#include <glm/glm.hpp>

namespace blu::core {
Engine::Engine(blu::core::Window* window) {
  window_ = window;
  input_ = {};
  if (!input_.LoadKeybinds()) {
    SetDefaultKeybinds();
  }
  prev_mouse_input_ = input_.GetMousePos();
}

Engine::~Engine() {}

void Engine::LoadScene(const eastl::string& scene_name) {
  camera_ = new components::Camera(
      components::Transform(glm::vec3(0, 0, 0), glm::vec3(0, 0, 0),
                            glm::vec3(1, 1, 1)),
      window_->GetAspectRatio(), 45, 1, 500);

  eastl::vector<glm::vec3> vertices = {
      {-1, -1, 0.5},   // 0
      {1, -1, 0.5},    // 1
      {-1, 1, 0.5},    // 2
      {1, 1, 0.5},     // 3
      {-1, -1, -0.5},  // 4
      {1, -1, -0.5},   // 5
      {-1, 1, -0.5},   // 6
      {1, 1, -0.5},    // 7
  };
  eastl::vector<uint32_t> indices{
      2,
      6,
      7,
      2,
      3,
      7,  // Top

      0,
      4,
      5,
      0,
      1,
      5,  // Bottom
      0,
      2,
      6,
      0,
      4,
      6,
      // Left
      // Right
      1,
      3,
      7,
      1,
      5,
      7,
      // Front
      0,
      2,
      3,
      0,
      1,
      3,
      // Back
      4,
      6,
      7,
      4,
      5,
      7,
  };

  models_.push_back(
      Model({glm::vec3(0, 0, 0), glm::vec3(0, 0, 0), glm::vec3(1, 1, 1)},
            Mesh(vertices, indices)));
}

// TEMP
void Engine::Update() {
  auto cam_front = camera_->GetTransform().Front();

  float move_speed = 0.02;

  if (input_.IsActionPressed("Forward")) {
    camera_->GetTransform().AddToPosition(cam_front * move_speed);
  }
  if (input_.IsActionPressed("Left")) {
    camera_->GetTransform().AddToPosition(
        -glm::normalize(glm::cross(cam_front, glm::vec3(0.0f, 1.0f, 0.0f))) *
        move_speed);
  }
  if (input_.IsActionPressed("Right")) {
    camera_->GetTransform().AddToPosition(
        glm::normalize(glm::cross(cam_front, glm::vec3(0.0f, 1.0f, 0.0f))) *
        move_speed);
  }
  if (input_.IsActionPressed("Back")) {
    camera_->GetTransform().AddToPosition(-cam_front * move_speed);
  }

  glm::vec2 mouse_pos = input_.GetMousePos();
  glm::vec2 mouse_pos_diff = prev_mouse_input_ - mouse_pos;

  if (input_.IsActionPressed("LMB")) {
    camera_->GetTransform().AddToRotation(
        glm::vec3(mouse_pos_diff.y * 4, -mouse_pos_diff.x * 4, 0.0f));
  }
}

Engine::RenderData Engine::GetRenderData() {
  eastl::vector<glm::mat4> matrices(3);

  matrices[0] = camera_->GetTransform().GetTransformMat();
  matrices[1] = camera_->GetPerspectiveMat();
  matrices[2] = models_[0].transform.GetTransformMat();

  return RenderData(matrices);
}

void Engine::SetDefaultKeybinds() {
  input_.RegisterKeyBind("LMB", GLFW_MOUSE_BUTTON_LEFT);

  input_.RegisterKeyBind("Forward", GLFW_KEY_W, GLFW_KEY_UP);
  input_.RegisterKeyBind("Left", GLFW_KEY_A, GLFW_KEY_LEFT);
  input_.RegisterKeyBind("Right", GLFW_KEY_D, GLFW_KEY_RIGHT);
  input_.RegisterKeyBind("Back", GLFW_KEY_S, GLFW_KEY_DOWN);
}
}  // namespace blu::core