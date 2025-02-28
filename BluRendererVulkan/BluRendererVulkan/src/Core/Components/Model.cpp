#include "Model.h"

#include <iostream>

// Needs to distiguish vertex information based on provided info
blu::core::components::Model::Model(eastl::string filepath) {
  Assimp::Importer importer;

  const aiScene* scene =
      importer.ReadFile(filepath.c_str(), aiProcess_Triangulate |
                                              aiProcess_JoinIdenticalVertices |
                                              aiProcess_SortByPType);
  if (scene == nullptr) {
    std::cerr << importer.GetErrorString() << std::endl;
    return;
  }

#ifdef MODEL_DEBUG
  std::cout << "Filepath Name: " << filepath << std::endl;
  std::cout << "Model Name: " << scene->mName.C_Str() << std::endl;
  std::cout << "Mesh Count: " << scene->mNumMeshes << std::endl;
  std::cout << "Texture Count: " << scene->mNumTextures << std::endl;
  std::cout << "Materials Count: " << scene->mNumMaterials << std::endl;
#endif

  for (uint32_t i = 0; i < scene->mNumMeshes; i++) {
    auto& mesh = scene->mMeshes[i];
    if (mesh->HasPositions()) {
      vertex_count_ = mesh->mNumVertices;
      vertex_data_size_ = sizeof(float) * 3 * vertex_count_;
      vertex_data_ = new char[vertex_data_size_];
      memcpy(vertex_data_, mesh->mVertices, vertex_data_size_);
    }
    if (mesh->HasFaces()) {
      eastl::vector<uint32_t> indices;
      for (uint32_t j = 0; j < mesh->mNumFaces; j++) {
        indices.push_back(mesh->mFaces[j].mIndices[0]);
        indices.push_back(mesh->mFaces[j].mIndices[1]);
        indices.push_back(mesh->mFaces[j].mIndices[2]);
      }
      index_count_ = indices.size();
      index_data_size_ = sizeof(uint32_t) * index_count_;
      index_data_ = new char[index_data_size_];
      memcpy(index_data_, indices.data(), index_data_size_);
    }
    if (mesh->HasNormals()) {
      normal_data_size_ = sizeof(float) * 3 * vertex_count_;
      normal_data_ = new char[normal_data_size_];
      memcpy(normal_data_, mesh->mNormals, normal_data_size_);
    }
  }

  return;
}

void blu::core::components::Model::Delete() {
  delete vertex_data_;
  delete index_data_;
  delete normal_data_;
}