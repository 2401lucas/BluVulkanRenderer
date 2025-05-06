#ifndef ENGINE_H
#define ENGINE_H

#include <glm/vec3.hpp>

#include "../External/Input.h"
#include "../External/Window.h"
#include "../Rendering/ForwardRenderer.h"
#include "../Rendering/RenderData.h"
#include "Components/Camera.h"
#include "Components/Model.h"
#include "Components/Transform.h"
#include "GameManager.h"
#include "TestGameManager.h"

namespace blu::core {
class Script { /*
  public:
   virtual void OnEnable();
   virtual void OnDisable();
   virtual void Start();
   virtual void Update();*/
};

// Compact Render Data
// I would like optimize the memory layout by seperating data based on
// components, but specifically the data required by the renderer
// Example, When creating Transforms, it requires a pointer to where the model
// matrix is stored
// The model matrix is stored in a vector with other models_, so
// when sending data the the GPU, instead of assembling all of the data into a
// new array, we can just send it the pointer to the matrix array
class Engine {
 public:
  Engine(blu::core::Window*, ForwardRenderer*);
  ~Engine();

  void LoadScene(const eastl::string& scene_name);

  blu::game::components::Model* CreateModel(
      const eastl::string& filepath,
      blu::game::components::Transform);

  void Update(float frametime);
  void FixedUpdate(float frametime);

  void SetCameraAspectRatio(float aspect_ratio);
  RenderData GetRenderData();

 private:
  void SetDefaultKeybinds();

  Window* window_ = nullptr;
  ForwardRenderer* renderer_;
  KeybindManager* input_;

  blu::game::components::Camera* camera_ = nullptr;
  blu::game::TestGameManager game_manager_{};

  eastl::vector<blu::game::components::Model*> models_;
};
}  // namespace blu::core
#endif