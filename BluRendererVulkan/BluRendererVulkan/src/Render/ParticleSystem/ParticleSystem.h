#pragma once

#include <vulkan/vulkan_core.h>

#include "../Renderer/BaseRenderer.h"
#include "../ResourceManagement/ExternalResources/VulkanTexture.hpp"
#include "../ResourceManagement/VulkanResources/VulkanDevice.h"

// Could Create A ParticleSystemManager that manages the shared resources
// between each ParticleSystemInstance
struct ParticleSystemInfo {
  uint32_t particleCount;
  glm::vec3 accel;
  glm::vec3 maxVel;
  glm::vec3 minVel;
  float maxSpawnVariation;
  float lifespan;
  bool collision;
};

class ParticleSystem {
 private:
  bool emitting;

  vks::VulkanDevice* device;
  ParticleSystemInfo info;

  vks::Buffer storageBuffer;

  // Potentially Shared Resources (to be moved to manager)
  struct {
    vks::Texture2D particle;
    vks::Texture2D gradient;
  } textures;

  VkPipelineCache pipelineCache;
  // Particle rendering pipeline
  VkPipelineLayout graphicsPipelineLayout;
  VkPipeline graphicsPipeline;
  // Compute pipeline for updating particle positions
  VkPipelineLayout computePipelineLayout;
  VkPipeline computePipeline;
  VkDescriptorPool descriptorPool;
  VkDescriptorSetLayout graphicsDescriptorSetLayout;
  VkDescriptorSet graphicsDescriptorSet;
  VkDescriptorSetLayout computeDescriptorSetLayout;
  VkDescriptorSet computeDescriptorSet;

  VkSemaphore graphicsSemaphore;
  VkSemaphore computeSemaphore;

  // UBO containing particle system parameters
  vks::Buffer computeUniformBuffer;

  struct UniformData {  // Compute shader uniform block object
    float deltaT;       //		Frame delta time
    int32_t particleCount;
  };

  std::vector<UniformData> uniformDatas;

  // End of Shared Resources

  // Physics calculations can use velocity vector of particle to calculate
  // collisions (If enabled)
  struct Particle {
    glm::vec4 pos;          // XYZ Pos, W Type
    glm::vec4 vel;          // XYZ Vel, W Collision
    glm::vec4 gradientPos;  // TBD
  };

 public:
  ParticleSystem(BaseRenderer*, vks::VulkanDevice*, VkQueue,
                 ParticleSystemInfo);
  ~ParticleSystem();

  void setupDescriptorPool();
  void prepareStorageBuffer(VkQueue);
  void prepareUniformBuffer();
  void updateUniformBuffer(float frameTimer);
  void prepareGraphics(BaseRenderer* br);
  void prepareCommandBuffer();
  void prepareCompute(BaseRenderer* baseRenderer);
  void buildComputeBuffer(VkCommandBuffer cmdBuf, VkQueue queue);
  void draw(VkCommandBuffer);
  void update();
};