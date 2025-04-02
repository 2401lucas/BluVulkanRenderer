#include "Mesh.h"

blu::core::rendering::Mesh::Mesh(uint32_t material_index) {
  material_index_ = material_index;
}

blu::core::rendering::Mesh::~Mesh() {
  delete vertex_data_.data;
  delete index_data_.data;
  delete normal_data_.data;
  delete uv_data_.data;
}