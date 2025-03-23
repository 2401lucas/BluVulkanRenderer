#ifndef ENGINE_H
#define ENGINE_H

#include <glm/vec3.hpp>

#include "../Components/Camera.h"
#include "../Components/Model.h"
#include "../Components/Transform.h"
#include "../External/Input.h"
#include "../External/Window.h"
#include "../Rendering/ForwardRenderer.h"
#include "../Rendering/RenderData.h"

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
  Engine(blu::core::Window*);
  ~Engine();

  void LoadScene(const eastl::string& scene_name, ForwardRenderer* rndr);

  void Update();

  RenderData GetRenderData();

 private:
  void SetDefaultKeybinds();

  components::Camera* camera_ = nullptr;
  Window* window_ = nullptr;
  KeybindManager* input_;

  struct Model {
    uint32_t model_index;
    blu::core::components::Transform transform;
  };

  eastl::vector<Model> models_;
  float mouse_sens_ = 1;
  glm::vec2 prev_mouse_input_;
};
}  // namespace blu::core
#endif