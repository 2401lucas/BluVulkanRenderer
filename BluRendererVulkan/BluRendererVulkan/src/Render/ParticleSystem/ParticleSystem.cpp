#include "ParticleSystem.h"

ParticleSystem::ParticleSystem(vks::VulkanDevice* device,
                               ParticleSystemInfo info) {
  this->device = device;
  this->info = info;
}