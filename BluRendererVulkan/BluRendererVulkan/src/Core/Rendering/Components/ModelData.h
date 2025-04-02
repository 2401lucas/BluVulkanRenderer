#ifndef MODEL_DATA_H
#define MODEL_DATA_H

#include <EASTL/string.h>
#include <EASTL/vector.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <assimp/Importer.hpp>

#include "Material.h"
#include "Mesh.h"

namespace blu::core::rendering {
class ModelData {
 public:
  ModelData(eastl::string filepath);
  void Delete();

  eastl::vector<Mesh*>& GetMeshes() { return meshes_; };
  eastl::vector<Material>& GetMaterials() { return materials_; };

 private:
  eastl::string filepath_;

  eastl::vector<Mesh*> meshes_;
  eastl::vector<Material> materials_;
};
}  // namespace blu::core::components

#endif