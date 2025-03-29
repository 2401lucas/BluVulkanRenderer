#include "Engine.h"

#include <glm/glm.hpp>
#include <iostream>

#define MODEL_DEBUG 1

namespace blu::core {
Engine::Engine(blu::core::Window* window) {
  window_ = window;
  input_ = new KeybindManager(window);
  if (!input_->LoadKeybinds()) {
    SetDefaultKeybinds();
  }
  // prev_mouse_input_ = input_->GetMousePos();
  prev_mouse_input_ = {0, 0};
}

Engine::~Engine() {
  delete input_;
  delete camera_;
}

void Engine::LoadScene(const eastl::string& scene_name, ForwardRenderer* rndr) {
  camera_ = new components::Camera(
      new components::Transform(glm::vec3(0, 0, -5), glm::vec3(0, 0, 0),
                                glm::vec3(1, 1, 1)),
      window_->GetAspectRatio(), 45, 1, 500);

  auto model_index = rndr->LoadModel("assets/Cube/cube");
  if (model_index >= 0) {
    models_.push_back(
        Model(model_index,
              components::Transform(glm::vec3(0, 0, 0), glm::vec3(45, 45, 0),
                                    glm::vec3(0.5, 0.5, 0.5))));
  }
  model_index = rndr->LoadModel("assets/Cube/cube");
  if (model_index >= 0) {
    models_.push_back(
        Model(model_index,
              components::Transform(glm::vec3(-1, 1.5, 0), glm::vec3(0, 0, 0),
                                    glm::vec3(1, 1, 1))));
  }
  model_index = rndr->LoadModel("assets/Cube/cube");
  if (model_index >= 0) {
    models_.push_back(
        Model(model_index,
              components::Transform(glm::vec3(0, 3, 1), glm::vec3(0, 0, 0),
                                    glm::vec3(1, 1, 1))));
  }
}

void Engine::Update(float frametime) {
  camera_->Update();
  auto cam_front = camera_->GetTransform()->Front();

  float move_speed = 10 * frametime;

  models_[0].transform.AddToRotation(glm::vec3(36, 0, 0) * frametime);

  if (input_->IsActionPressed("W")) {
    camera_->GetTransform()->AddToPosition(cam_front * move_speed);
  }
  if (input_->IsActionPressed("A")) {
    camera_->GetTransform()->AddToPosition(
        -glm::normalize(glm::cross(cam_front, glm::vec3(0.0f, 1.0f, 0.0f))) *
        move_speed);
  }
  if (input_->IsActionPressed("D")) {
    camera_->GetTransform()->AddToPosition(
        glm::normalize(glm::cross(cam_front, glm::vec3(0.0f, 1.0f, 0.0f))) *
        move_speed);
  }
  if (input_->IsActionPressed("S")) {
    camera_->GetTransform()->AddToPosition(-cam_front * move_speed);
  }

  if (input_->IsActionPressed("LCTRL")) {
    camera_->GetTransform()->AddToPosition(glm::vec3(0.0f, 1.0f, 0.0f) *
                                           move_speed);
  }
  if (input_->IsActionPressed("SPACE")) {
    camera_->GetTransform()->AddToPosition(glm::vec3(0.0f, -1.0f, 0.0f) *
                                           move_speed);
  }
  if (input_->IsActionPressed("ESC")) {
    // TODO: QUIT
  }

  glm::vec2 mouse_pos = input_->GetMousePos();
  glm::vec2 mouse_pos_diff = prev_mouse_input_ - mouse_pos;
  prev_mouse_input_ = mouse_pos;

  auto mouse_sens = mouse_sens_ * frametime;

  if (input_->IsActionPressed("Mouse 1")) {
    camera_->GetTransform()->AddToRotation(glm::vec3(
        mouse_pos_diff.y * mouse_sens, -mouse_pos_diff.x * mouse_sens, 0.0f));
  }
}

void Engine::SetCameraAspectRatio(float aspect_ratio) {
  camera_->SetAspectRatio(aspect_ratio);
}

RenderData Engine::GetRenderData() {
  eastl::vector<uint32_t> model_ids;

  eastl::vector<glm::mat4> matrices(3);
  matrices[1] = camera_->GetTransform()->GetTransformMat();
  matrices[2] = camera_->GetPerspectiveMat();
  matrices[0] = matrices[2] * matrices[1];

  for (auto& m : models_) {
    matrices.push_back(m.transform.GetTransformMat());
    model_ids.push_back(m.model_index);
  }

  return RenderData(matrices, model_ids);
}

void Engine::SetDefaultKeybinds() {
  input_->RegisterKeyBind("Mouse 1", GLFW_MOUSE_BUTTON_LEFT);

  input_->RegisterKeyBind("W", GLFW_KEY_W, GLFW_KEY_UP);
  input_->RegisterKeyBind("A", GLFW_KEY_A, GLFW_KEY_LEFT);
  input_->RegisterKeyBind("D", GLFW_KEY_D, GLFW_KEY_RIGHT);
  input_->RegisterKeyBind("S", GLFW_KEY_S, GLFW_KEY_DOWN);
  input_->RegisterKeyBind("LCTRL", GLFW_KEY_LEFT_CONTROL);
  input_->RegisterKeyBind("SPACE", GLFW_KEY_SPACE);
  input_->RegisterKeyBind("ESC", GLFW_KEY_ESCAPE);
}
}  // namespace blu::core