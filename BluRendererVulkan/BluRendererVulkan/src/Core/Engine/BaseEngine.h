#pragma once
#include <vector>

namespace core_internal::engine {
class BaseEngine {
 private:
  std::vector<void*> renderInfo;
  void* physics;
  void* audio;

 protected:
  BaseEngine();
  ~BaseEngine();

  // (Pure Virtual)
  virtual void fixedUpdate() = 0;
  // (Pure Virtual)
  virtual void update(float deltaTime) = 0;

 public:
  void beginUpdate(float deltaTime);
  void beginFixedUpdate();


};
}  // namespace core_internal::engine