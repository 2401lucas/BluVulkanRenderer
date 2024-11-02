#ifndef ENGINE_H
#define ENGINE_H

#include <glm/vec3.hpp>

#include "../Components/Camera.h"
#include "../Components/Transform.h"
#include "../External/Input.h"
#include "../External/Window.h"

namespace blu::core {

class Script {
 public:
  virtual void OnEnable();
  virtual void OnDisable();
  virtual void Start();
  virtual void Update();
};

// Seperate Components
// Compact Render Data
// Camera Class
class Engine {
 public:
  Engine(blu::core::Window*);
  ~Engine();

  struct Mesh;
  struct Model;
  struct RenderData;

  void LoadScene(const eastl::string& scene_name);

  void Update();

  RenderData GetRenderData();

  struct Mesh {
    eastl::vector<glm::vec3> vertices;
    eastl::vector<uint32_t> indices;
  };

  struct Model {
    blu::core::components::Transform transform;
    Mesh mesh;
    Script script;
  };

  struct RenderData {
    // Matrix[0] - Camera View
    // Matrix[1] - Camera Perspective
    // Matrix[2...] - Model Position
    eastl::vector<glm::mat4> matrices;
  };

 private:
  void SetDefaultKeybinds();

  components::Camera* camera_ = nullptr;
  Window* window_ = nullptr;
  KeybindManager input_;

  glm::vec2 prev_mouse_input_;
  eastl::vector<Model> models_;
};
}  // namespace blu::core
#endif