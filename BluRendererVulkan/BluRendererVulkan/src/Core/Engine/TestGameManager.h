#ifndef TEST_GAME_MANAGER_HPP
#define TEST_GAME_MANAGER_HPP

#include "GameManager.h"

namespace blu::game {
class TestGameManager : public blu::game::GameManager {
 public:
  TestGameManager() {};
  TestGameManager(
      eastl::function<blu::game::components::Model*(
          const eastl::string& filepath, components::Transform transform)>
          model_creation_callback,
      blu::core::KeybindManager* input, blu::game::components::Camera* camera);
  ~TestGameManager();

  virtual void Start();
  virtual void Update(float delta_time);
  virtual void FixedUpdate();

 private:
  components::Model* main_model_;
};
}  // namespace blu::game

#endif