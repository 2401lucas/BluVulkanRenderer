#pragma once
#pragma once
#include <string>
#include <unordered_map>

#include "../Image/Image.h"
#include "../Textures/TextureManager.h"

struct MetallicProperty {
  Texture metallicMap;
  float metallic;
  float smoothness;
};
struct NormalProperty {
  Texture normalMap;
  float amplitude;
};

struct BasicMaterial {
  std::string materialPath;
  std::string shaderPath;
  Texture albedoMap;
  MetallicProperty metallicProperty;
  NormalProperty normalProperty;
  bool specularHighlights;
  bool enableGPUInstancing;
};

struct PBRMaterial {};

class MaterialManager {
 public:
  MaterialManager();
  void cleanup(Device* device);
  void preregisterBasicMaterials(Device* device, CommandPool* commandPool,
                                 std::vector<std::string> matPaths);
  uint32_t registerBasicMaterial(Device* device, CommandPool* commandPool,
                                 std::string matPath);
  std::vector<BasicMaterial> getBasicMaterials();

  TextureManager* textureManager;
 private:
  // Contains all Material Data that is to be used with the UberShader
  std::unordered_map<std::string, uint32_t> basicUberMaterialIndices;
  std::vector<BasicMaterial> basicUberMaterials;
};