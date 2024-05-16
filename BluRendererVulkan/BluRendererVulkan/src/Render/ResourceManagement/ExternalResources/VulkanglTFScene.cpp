#include "VulkanglTFScene.h"

void vkglTF::Scene::loadFromFile(std::string filename,
                                 vks::VulkanDevice* device,
                                 VkQueue transferQueue,
                                 uint32_t fileLoadingFlags, float scale) {
  tinygltf::Model gltfModel;
  tinygltf::TinyGLTF gltfContext;

  std::string error;
  std::string warning;

  this->device = device;

  bool binary = false;
  size_t extpos = filename.rfind('.', filename.length());
  if (extpos != std::string::npos) {
    binary = (filename.substr(extpos + 1, filename.length() - extpos) == "glb");
  }

  bool fileLoaded = binary
                        ? gltfContext.LoadBinaryFromFile(
                              &gltfModel, &error, &warning, filename.c_str())
                        : gltfContext.LoadASCIIFromFile(
                              &gltfModel, &error, &warning, filename.c_str());


}