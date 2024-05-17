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

  // Potentially Shared Resources
  struct {
    vks::Texture2D particle;
    vks::Texture2D gradient;
  } textures;

  VkPipelineCache pipelineCache;
  VkDescriptorPool descriptorPool;
  struct Graphics {
    VkDescriptorSetLayout
        descriptorSetLayout;  // Particle system rendering shader binding layout
    VkDescriptorSet descriptorSet;  // Particle system rendering shader bindings
    VkPipelineLayout pipelineLayout;  // Layout of the graphics pipeline
    VkPipeline pipeline;              // Particle rendering pipeline
    VkSemaphore
        semaphore;  // Execution dependency between compute & graphic submission
  } graphics;

  struct Compute {
    VkQueue queue;  // Separate queue for compute commands (queue family may
                    // differ from the one used for graphics)
    VkCommandPool commandPool;  // Use a separate command pool (queue family may
                                // differ from the one used for graphics)
    VkCommandBuffer commandBuffer;  // Command buffer storing the dispatch
                                    // commands and barriers
    VkSemaphore
        semaphore;  // Execution dependency between compute & graphic submission
    VkDescriptorSetLayout descriptorSetLayout;  // Compute shader binding layout
    VkDescriptorSet descriptorSet;              // Compute shader bindings
    VkPipelineLayout pipelineLayout;  // Layout of the compute pipeline
    VkPipeline pipeline;  // Compute pipeline for updating particle positions
    vks::Buffer uniformBuffer;  // Uniform buffer object containing particle
                                // system parameters
    struct UniformData {        // Compute shader uniform block object
      float deltaT;             //		Frame delta time
      int32_t particleCount;
    } uniformData;
  } compute;
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
  void buildCommandBuffer();
  void prepareCompute(BaseRenderer* baseRenderer);
  void buildComputeBuffer();
  void draw(VkCommandBuffer);
  void update();
};