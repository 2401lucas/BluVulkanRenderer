#include "TestGameManager.h"

blu::game::TestGameManager::TestGameManager(
    eastl::function<blu::game::components::Model*(
        const eastl::string& filepath, components::Transform transform)>
        model_creation_callback,
    blu::core::KeybindManager* input, blu::game::components::Camera* camera)
    : GameManager(model_creation_callback, input, camera) {}

blu::game::TestGameManager::~TestGameManager() {}

void blu::game::TestGameManager::Start() {
  main_model_ = CreateModel("Cube/cube.glTF");
  CreateModel("Cube/cube.glTF",
              blu::game::components::Transform(false, glm::vec3(2, 2, 0),
                                               glm::vec3(45, 45, 0),
                                               glm::vec3(0.5, 0.5, 0.5)));
  CreateModel("Cube/cube.glTF", blu::game::components::Transform(
                                    false, glm::vec3(0, 3, 1),
                                    glm::vec3(0, 0, 0), glm::vec3(1, 1, 1)));
  CreateModel("Avocado/avocado.glTF",
              blu::game::components::Transform(false, glm::vec3(5, 0, 0),
                                               glm::vec3(0, 180, 0),
                                               glm::vec3(100, 100, 100)));
}

void blu::game::TestGameManager::Update(float delta_time) {
  auto cam_front = camera_->GetTransform()->Front();
  float move_speed = 10 * delta_time;

  main_model_->transform.AddToRotation(glm::vec3(36, 0, 0) * delta_time);

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

  glm::vec2 mouse_pos_diff = input_->GetMousePosDiff();
  mouse_pos_diff *= delta_time;

  if (input_->IsActionPressed("Mouse 1")) {
    camera_->GetTransform()->AddToRotation(
        glm::vec3(mouse_pos_diff.y, -mouse_pos_diff.x, 0.0f));
  }
}

void blu::game::TestGameManager::FixedUpdate() {}
