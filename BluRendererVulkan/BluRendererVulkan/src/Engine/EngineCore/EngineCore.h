#pragma once
#include "../../Render/Renderer/RenderSceneData.h"
#include "../../Render/Textures/TextureManager.h"
#include "../Entity/EntityManager.h"
#include "../Input/Input.h"
#include "../Scene/SceneManager.h"
#include "../src/Render/Renderer/RenderManager.h"

struct EngineTime {
  float deltaTime;
  float totalTime;
};

class EngineCore {
 public:
  EngineCore(GLFWwindow* window, const VkApplicationInfo& appInfo,
             DeviceSettings deviceSettings);
  RenderSceneData update(const float& time, InputData inputData,
                         bool frameBufferResized);
  void fixedUpdate(const EngineTime& time);
  void loadScene(const char* scenePath);

 private:
  SceneManager* sceneManager;
  RenderManager* renderManager;
  EntityManager* entityManager;
};