#ifndef MESH_H
#define MESH_H

#include <cstdint>

namespace blu::core::components {
class Mesh {
 public:
  struct MeshData {
    uint32_t count = 0;
    char* data = nullptr;
    uint32_t data_size = 0;
  };

  Mesh(uint32_t material_index);
  ~Mesh();

  MeshData& GetVertexData() { return vertex_data_; }
  MeshData& GetIndexData() { return index_data_; }
  MeshData& GetNormalData() { return normal_data_; }
  MeshData& GetUVData() { return uv_data_; }
  uint32_t& GetMaterialIndex() { return material_index_; };

 private:
  MeshData vertex_data_;
  MeshData index_data_;
  MeshData normal_data_;
  MeshData uv_data_;

  uint32_t material_index_;
};
}  // namespace blu::core::components
#endif