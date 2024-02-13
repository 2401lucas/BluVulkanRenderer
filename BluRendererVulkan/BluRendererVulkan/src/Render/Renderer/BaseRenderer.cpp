#include "BaseRenderer.h"

#include "../../../include/RenderConst.h"

BaseRenderer::BaseRenderer(GLFWwindow* window, const VkApplicationInfo& appInfo,
                           DeviceSettings deviceSettings) {
  vkInstance = new VulkanInstance(appInfo);
  device = new Device(window, vkInstance, deviceSettings);
  frameIndex = RenderConst::MAX_FRAMES_IN_FLIGHT;
  swapchain = new Swapchain(device);
  meshManager = new MeshBufferManager(device);
  textureManager = new TextureManager();
  graphicsCommandPool = new CommandPool(
      device, device->findQueueFamilies().graphicsFamily.value(),
      VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
  graphicsCommandPool->createCommandBuffers(device);
}

void BaseRenderer::cleanup() {
  meshManager->cleanup(device);
  delete meshManager;
  textureManager->cleanup(device);
  delete textureManager;
  graphicsCommandPool->cleanup(device);
  delete graphicsCommandPool;
  swapchain->cleanup(device);
  delete swapchain;
  device->cleanup(vkInstance);
  delete device;
  vkInstance->cleanup();
  delete vkInstance;
}

void BaseRenderer::draw(const bool& framebufferResized,
                        RenderSceneData& sceneData) {
  frameIndex = (frameIndex + 1) % RenderConst::MAX_FRAMES_IN_FLIGHT;
}

void BaseRenderer::registerMesh(MeshData meshData) {
  meshManager->registerMesh(meshData);
  meshManager->rebuildBuffers(device, graphicsCommandPool);
}
void BaseRenderer::registerMeshes(std::vector<MeshData> meshes) {
  for (auto& mesh : meshes) {
    meshManager->registerMesh(mesh);
  }
  meshManager->rebuildBuffers(device, graphicsCommandPool);
}

void BaseRenderer::registerTextures(std::vector<TextureInfo> textureInfos) {
  textureManager->loadTextures(device, graphicsCommandPool, textureInfos);
}

uint32_t BaseRenderer::getTextureIndex(TextureInfo info) {
  return textureManager->getTextureIndex(info);
}