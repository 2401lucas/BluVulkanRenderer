#include "MaterialManager.h"
#include "../../Engine/FileManagement/FileManager.h"

MaterialManager::MaterialManager() { textureManager = TextureManager(); }

void MaterialManager::cleanup(Device* device) {
  textureManager.cleanup(device);
}

void MaterialManager::preregisterBasicMaterials(
    Device* device, CommandPool* commandPool, std::vector<std::string> matPaths) {
  for (auto& matPath : matPaths) {
    if (basicUberMaterialsIndex.count(matPath) == 0) {
      basicUberMaterialsIndex[matPath] = basicUberMaterials.size();
      auto file = FileManager::readFile(matPath);
      textureManager.registerTexture(device, commandPool, matPath.albedoMap);
      textureManager.registerTexture(device, commandPool,
                                     matPath.metallicProperty.metallicMap);
      textureManager.registerTexture(device, commandPool,
                                     matPath.normalProperty.normalMap);
      basicUberMaterials.push_back(matPath);
    }
  }
}

uint32_t MaterialManager::registerBasicMaterial(Device* device,
                                                CommandPool* commandPool,
                                                BasicMaterial mat) {
  if (basicUberMaterialsIndex.count(mat.materialPath) == 0) {
    basicUberMaterialsIndex[mat.materialPath] = basicUberMaterials.size();
    basicUberMaterials.push_back(mat);
    textureManager.registerTexture(device, commandPool, mat.albedoMap);
    textureManager.registerTexture(device, commandPool,
                                   mat.metallicProperty.metallicMap);
    textureManager.registerTexture(device, commandPool,
                                   mat.normalProperty.normalMap);
    return basicUberMaterials.size() - 1;
  } else {
    return basicUberMaterialsIndex[mat.materialPath];
  }
}

std::vector<BasicMaterial> MaterialManager::getBasicMaterials() {
  return basicUberMaterials;
}