#include "MaterialManager.h"

#include "../../Engine/FileManagement/FileManager.h"

MaterialManager::MaterialManager() { textureManager = new TextureManager(); }

void MaterialManager::cleanup(Device* device) {
  textureManager->cleanup(device);
}

void MaterialManager::preregisterBasicMaterials(
    Device* device, CommandPool* commandPool,
    std::vector<std::string> matPaths) {
  for (auto& matPath : matPaths) {
    if (basicUberMaterialIndices.count(matPath) == 0) {
      basicUberMaterialIndices[matPath] = basicUberMaterials.size();
      BasicMaterial basicMat;
      basicMat.albedoMap.filePath = "textures/blue.png";
      // FileManager::readStructFromFile<BasicMaterial>(matPath, basicMat);
      textureManager->registerTexture(device, commandPool, basicMat.albedoMap);
      //textureManager.registerTexture(device, commandPool,
                                     //basicMat.metallicProperty.metallicMap);
      //textureManager.registerTexture(device, commandPool,
                                     //basicMat.normalProperty.normalMap);
      basicUberMaterials.push_back(basicMat);
    }
  }
}

uint32_t MaterialManager::registerBasicMaterial(Device* device,
                                                CommandPool* commandPool,
                                                std::string matPath) {
  if (basicUberMaterialIndices.count(matPath) == 0) {
    basicUberMaterialIndices[matPath] = basicUberMaterials.size();
    BasicMaterial basicMat;
    basicMat.albedoMap.filePath = "textures/blue.png";
    // FileManager::readStructFromFile<BasicMaterial>(matPath, basicMat);
    textureManager->registerTexture(device, commandPool, basicMat.albedoMap);
    // textureManager.registerTexture(device, commandPool,
    // basicMat.metallicProperty.metallicMap);
    // textureManager.registerTexture(device, commandPool,
    // basicMat.normalProperty.normalMap);
    basicUberMaterials.push_back(basicMat);

    return basicUberMaterials.size() - 1;
  } else {
    return basicUberMaterialIndices[matPath];
  }
}

std::vector<BasicMaterial> MaterialManager::getBasicMaterials() {
  return basicUberMaterials;
}