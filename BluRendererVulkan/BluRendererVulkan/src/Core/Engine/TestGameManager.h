#ifndef TEST_GAME_MANAGER_HPP
#define TEST_GAME_MANAGER_HPP

#include "GameManager.h"

namespace blu::game {
class TestGameManager : public blu::game::GameManager {
 public:
  TestGameManager() {};
  TestGameManager(GameManagerCallbackHelper callback_helper,
      blu::core::KeybindManager* input, blu::game::components::Camera* camera);
  ~TestGameManager();

  virtual void Start();
  virtual void Update(float delta_time);
  virtual void FixedUpdate(float delta_time);

 private:
  components::Model* main_model_;
};
}  // namespace blu::game

#endif