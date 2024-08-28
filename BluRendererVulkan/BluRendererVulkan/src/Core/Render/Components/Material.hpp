#pragma once
#include <glm/ext/vector_float4.hpp>

namespace core_internal::rendering::components {
struct alignas(16) GPUMaterial {
  glm::vec4 baseColorFactor;
  glm::vec4 emissiveFactor;
  glm::vec4 diffuseFactor;
  glm::vec4 specularFactor;
  float workflow;
  int colorTextureSet;
  int physicalDescriptorTextureSet;
  int normalTextureSet;
  int occlusionTextureSet;
  int emissiveTextureSet;
  float metallicFactor;
  float roughnessFactor;
  float alphaMask;
  float alphaMaskCutoff;
  float emissiveStrength;
};
}  // namespace core_internal::rendering::components

namespace core::engine::components {
enum ShaderWorkflows {
  SHADER_WORKFLOW_PBR_METALLIC_ROUGHNESS = 0,
  SHADER_WORKFLOW_PBR_SPECULAR_GLOSSINESS = 1
};

struct Material {
  glm::vec4 baseColorFactor;
  glm::vec4 emissiveFactor;
  glm::vec4 diffuseFactor;
  glm::vec4 specularFactor;
  float workflow;
  int colorTextureSet;
  int physicalDescriptorTextureSet;
  int normalTextureSet;
  int occlusionTextureSet;
  int emissiveTextureSet;
  float metallicFactor;
  float roughnessFactor;
  float alphaMask;
  float alphaMaskCutoff;
  float emissiveStrength;
};
}  // namespace core::engine::components