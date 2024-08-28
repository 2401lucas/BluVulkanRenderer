#pragma once
#include "../../libraries/GLTF/tiny_gltf.h"

namespace tools {
class FileManager {
 public:
  FileManager() = delete;
  ~FileManager() = delete;

  static tinygltf::Model loadglTFScene(std::string filePath, bool isBinary) {
    tinygltf::Model newModel;
    tinygltf::TinyGLTF gltfContext;
    std::string error, warning;

    bool fileLoaded = isBinary
                          ? gltfContext.LoadBinaryFromFile(
                                &newModel, &error, &warning, filePath.c_str())
                          : gltfContext.LoadASCIIFromFile(
                                &newModel, &error, &warning, filePath.c_str());
  };
};
}  // namespace tools