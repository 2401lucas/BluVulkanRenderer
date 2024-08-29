#pragma once
#include <vector>

#include "../Render/Components/Camera.hpp"
#include "../Render/Components/Input.hpp"

namespace core_internal::engine {
// Base Engine Class
// EXAMPLES: (Derived Class becomes more of a game manager than an actual engine?)
// Game Engine: Child Script Updates
// Physics Engine: Modifying huge list of objects with minimal materials.
class BaseEngine {
 private:
  std::vector<void*> renderInfo;
  void* physics;
  void* audio;

 protected:
  core::engine::components::Camera camera;

  BaseEngine();
  ~BaseEngine();

  // (Pure Virtual)
  virtual void fixedUpdate() = 0;
  // (Pure Virtual)
  virtual void update(float deltaTime) = 0;

 public:
  void onResized(float aspectRatio);
  void beginUpdate(float deltaTime, InputData input);
  void beginFixedUpdate();
};
}  // namespace core_internal::engine