#pragma once
#include "../Buffer/MeshBufferManager.h"
#include "../Device/Device.h"
#include "../Materials/MaterialManager.h"
#include "../Swapchain/Swapchain.h"
#include "../VkInstance/VulkanInstance.h"
#include "RenderSceneData.h"

class BaseRenderer {
 protected:
  BaseRenderer(GLFWwindow*, const VkApplicationInfo&, DeviceSettings);

 public:
  virtual void cleanup();
  virtual void draw(const bool& framebufferResized, RenderSceneData&);
  void registerMesh(MeshData mesh);
  void registerMeshes(std::vector<MeshData> meshes);
  uint32_t registerMaterial(std::string matPath);

 protected:
  struct InstancedData {
    glm::vec3 pos;
    glm::vec3 rot;
    glm::vec3 scale;
    uint32_t texIndex;
  };

  uint32_t frameIndex;
  VulkanInstance* vkInstance;
  Device* device;
  Swapchain* swapchain;
  CommandPool* graphicsCommandPool;

  MeshBufferManager* meshManager;
  MaterialManager* materialManager;
};