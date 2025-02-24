#include "Engine.h"

#include <glm/glm.hpp>

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
      new components::Transform(glm::vec3(0, 0, 0), glm::vec3(0, 0, 0),
                                glm::vec3(1, 1, 1)),
      window_->GetAspectRatio(), 45, 1, 500);

  auto model_index = rndr->LoadModel("assets/Cube/cube.glTF");
  if (model_index >= 0) {
    models_.push_back(
        Model(model_index,
              components::Transform(glm::vec3(0, 0, 0), glm::vec3(0, 0, 0),
                                    glm::vec3(1, 1, 1))));
  }
}

// Model Creation
// Read from file
// ...
// Upload to GPU
// ...
// For rendering, I think an index is needed?
//
// So essentially I need the data on the GPU which requires use of the renderer.
// I would prefer keeping the modules seperated but I am unsure that is
// required. Hypthetically if they aren't seperated, we can add operations to a
// queue to be processed at the start of the next frame. Example for model
// loading would be:
// LoadModel(MeshData /*Allows for proc gen*/)) & LoadModel("FilePath")
// Engine model holds filepath and leaves file loading
// operations for Renderer, which could work async to main thread.
// LoadModel would return an index for the respective model data, either based
// on existing data existing with said filepath or, assuming a new model, would
// assign a new index. This index would point to all respective model data
// including texture indices, mesh indices & used in generating draw commands
void Engine::Update() {
  auto cam_front = camera_->GetTransform()->Front();

  float move_speed = 0.02;

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

  glm::vec2 mouse_pos = input_->GetMousePos();
  glm::vec2 mouse_pos_diff = prev_mouse_input_ - mouse_pos;

  if (input_->IsActionPressed("LMB")) {
    camera_->GetTransform()->AddToRotation(
        glm::vec3(mouse_pos_diff.y * 4, -mouse_pos_diff.x * 4, 0.0f));
  }
}

RenderData Engine::GetRenderData() {
  eastl::vector<glm::mat4> matrices(3);

  matrices[0] = camera_->GetTransform()->GetTransformMat();
  matrices[1] = camera_->GetPerspectiveMat();
  matrices[2] = models_[0].transform.GetTransformMat();

  return RenderData(matrices);
}

void Engine::SetDefaultKeybinds() {
  input_->RegisterKeyBind("Mouse 1", GLFW_MOUSE_BUTTON_LEFT);

  input_->RegisterKeyBind("W", GLFW_KEY_W, GLFW_KEY_UP);
  input_->RegisterKeyBind("A", GLFW_KEY_A, GLFW_KEY_LEFT);
  input_->RegisterKeyBind("D", GLFW_KEY_D, GLFW_KEY_RIGHT);
  input_->RegisterKeyBind("S", GLFW_KEY_S, GLFW_KEY_DOWN);
}
}  // namespace blu::core