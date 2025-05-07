#ifndef GAME_MANAGER_HPP
#define GAME_MANAGER_HPP

#include <EASTL/functional.h>
#include <eastl/string.h>

#include "../External/Input.h"
#include "Components/Light.h"
#include "GameManagerCallbackHelper.h"
#include "components/Camera.h"
#include "components/Model.h"
#include "components/Transform.h"

namespace blu::game {
class GameManager {
 public:
  GameManager() {};
  GameManager(GameManagerCallbackHelper callback_helper,
              blu::core::KeybindManager* input,
              blu::game::components::Camera* camera) {
    model_creation_callback_ = callback_helper.model_creation_callback;
    light_creation_callback_ = callback_helper.light_creation_callback;
    input_ = input;
    camera_ = camera;
  };
  ~GameManager() {};

  virtual void Start() {};
  virtual void Update(float delta_time) {};
  virtual void FixedUpdate() {};

 protected:
  blu::core::KeybindManager* input_;
  blu::game::components::Camera* camera_;

  blu::game::components::Model* CreateModel(
      const eastl::string& filepath,
      components::Transform transform =
          blu::game::components::Transform::Default()) {
    return model_creation_callback_(filepath, transform);
  }

  blu::game::components::Light* CreateLight(
      components::Transform transform =
          blu::game::components::Transform::Default()) {
    return light_creation_callback_(transform);
  }

 private:
  eastl::function<blu::game::components::Model*(
      const eastl::string& filepath, components::Transform transform)>
      model_creation_callback_;
  eastl::function<blu::game::components::Light*(
      components::Transform transform)>
      light_creation_callback_;
};
}  // namespace blu::game

#endif