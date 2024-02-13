#pragma once
#include "../../Engine/Scene/Scene.h"
#include "BaseRenderer.h"

enum RendererType { Default };

class RenderManager {
 public:
  RenderManager(const RendererType& rType, GLFWwindow* window,
                const VkApplicationInfo& appInfo, DeviceSettings deviceSettings,
                const SceneDependancies* sceneDependancies);
  void cleanup();
  void draw(const bool& framebufferResized, RenderSceneData& sceneData);

  BaseRenderer* getRenderer();

 private:
  BaseRenderer* renderer;
};