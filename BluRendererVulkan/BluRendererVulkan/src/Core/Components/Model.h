#ifndef MODEL_H
#define MODEL_H

#include <EASTL/string.h>
#include <EASTL/vector.h>
#include <assimp/postprocess.h>  // Post processing flags
#include <assimp/scene.h>        // Output data structure

#include <assimp/Importer.hpp>  // C++ importer interface

namespace blu::core::components {
class Model {
 public:
  Model(eastl::string filepath);
  void Delete();

  void* GetVertexData() { return vertex_data_; }
  uint32_t GetVertexDataSize() { return vertex_data_size_; }
  void* GetIndexData() { return index_data_; }
  uint32_t GetIndexDataSize() { return index_data_size_; }
  void* GetNormalData() { return normal_data_; }
  uint32_t GetNormalDataSize() { return normal_data_size_; }

  uint32_t GetVertexCount() { return vertex_count_; }
  uint32_t GetIndexCount() { return index_count_; }

 private:
  eastl::string filepath_;

  uint32_t vertex_count_;
  uint32_t index_count_;

  char* vertex_data_ = nullptr;
  uint32_t vertex_data_size_ = 0;
  char* index_data_ = nullptr;
  uint32_t index_data_size_ = 0;
  char* normal_data_ = nullptr;
  uint32_t normal_data_size_ = 0;
  // One Model can have several Mesh
  // Each mesh only has 1 material
  // Draws will take 2 calls but
};
}  // namespace blu::core::components

#endif