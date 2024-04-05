#pragma once

#include <vulkan/vulkan_core.h>

#include <glm/glm.hpp>

#include "../ResourceManagement/VulkanResources/VulkanDevice.h"

struct ParticleSystemInfo {
  uint32_t particleCount;
  glm::vec3 accel;
  glm::vec3 maxVel;
  glm::vec3 minVel;
  float lifespan;
  bool collision;
};

class ParticleSystem {
 private:
  bool emitting;

  vks::VulkanDevice* device;
  ParticleSystemInfo info;

  // Physics calculations can use velocity vector of particle to calculate
  // collisions (If enabled)
  struct Particle {
    glm::vec4 pos;          // XYZ Pos, W Type
    glm::vec4 vel;          // XYZ Vel, W Collision
    glm::vec4 gradientPos;  // TBD
  };

 public:
  ParticleSystem(vks::VulkanDevice*, ParticleSystemInfo);
  ~ParticleSystem();

  void init();
  void draw(VkCommandBuffer);
  void update();
};