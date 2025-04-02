#include "Engine.h"

#include <glm/glm.hpp>
#include <iostream>

namespace blu::core {
Engine::Engine(blu::core::Window* window, ForwardRenderer* renderer) {
  window_ = window;
  renderer_ = renderer;
  input_ = new KeybindManager(window);
  if (!input_->LoadKeybinds()) {
    SetDefaultKeybinds();
  }

  prev_mouse_input_ = {0, 0};
}

Engine::~Engine() {
  for (auto& model : models_) {
    delete model;
  }
  delete camera_;
  delete input_;
}

void Engine::LoadScene(const eastl::string& scene_name) {
  camera_ = new blu::game::components::Camera(
      new blu::game::components::Transform(
          true, glm::vec3(0, 0, -5), glm::vec3(0, 0, 0), glm::vec3(1, 1, 1)),
      window_->GetAspectRatio(), 45, 1, 500);

  game_manager_ = blu::game::TestGameManager::TestGameManager(
      [this](const eastl::string& filepath,
             blu::game::components::Transform transform) {
        return this->CreateModel(filepath, transform);
      },
      input_, camera_);

  game_manager_.Start();
}

blu::game::components::Model* Engine::CreateModel(
    const eastl::string& filepath, blu::game::components::Transform transform) {
  auto model_index = renderer_->LoadModel(filepath);

  if (model_index >= 0) {
    blu::game::components::Model* new_model =
        new blu::game::components::Model(model_index, transform);
    models_.push_back(new_model);
    return new_model;
  }

  assert(false);
}

void Engine::Update(float frametime) {
  camera_->Update();
  game_manager_.Update(frametime);
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
    matrices.push_back(m->transform.GetTransformMat());
    model_ids.push_back(m->model_index);
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