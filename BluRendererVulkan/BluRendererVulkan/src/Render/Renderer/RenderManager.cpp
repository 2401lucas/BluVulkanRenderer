#include "RenderManager.h"

#include "DefaultRenderer.h"

RenderManager::RenderManager(const RendererType& rType, GLFWwindow* window,
                             const VkApplicationInfo& appInfo,
                             DeviceSettings ds, const SceneDependancies* sd) {
  switch (rType) {
    case Default:
      renderer = new DefaultRenderer(window, appInfo, ds, sd);
      break;
    default:
      renderer = nullptr;
      break;
  }
}

void RenderManager::cleanup() {
  renderer->cleanup();
  delete renderer;
}

void RenderManager::draw(const bool& framebufferResized,
                         RenderSceneData& sceneData) {
  renderer->draw(framebufferResized, sceneData);
}

BaseRenderer* RenderManager::getRenderer() { return renderer; }
