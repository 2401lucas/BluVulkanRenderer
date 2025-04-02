#include "Model.h"

#include <iostream>

// Needs to distiguish vertex information based on provided info
blu::core::components::Model::Model(eastl::string filepath) {
  Assimp::Importer importer;

  const aiScene* scene =
      importer.ReadFile(filepath.c_str(), aiProcess_Triangulate |
                                              aiProcess_JoinIdenticalVertices |
                                              aiProcess_FlipUVs);
  if (scene == nullptr) {
    std::cerr << importer.GetErrorString() << std::endl;
    return;
  }

#ifdef DEBUG_MODEL
  std::cout << "Filepath Name: " << filepath << std::endl;
  std::cout << "Model Name: " << scene->mName.C_Str() << std::endl;
  std::cout << "Mesh Count: " << scene->mNumMeshes << std::endl;
  std::cout << "Texture Count: " << scene->mNumTextures << std::endl;
  std::cout << "Materials Count: " << scene->mNumMaterials << std::endl;
#endif
  for (uint32_t i = 0; i < scene->mNumMeshes; i++) {
    auto& mesh = scene->mMeshes[i];
    Mesh* new_mesh = new Mesh(mesh->mMaterialIndex);
    if (mesh->HasPositions()) {
      Mesh::MeshData& vertex_data = new_mesh->GetVertexData();
      vertex_data.count = mesh->mNumVertices;
      vertex_data.data_size = sizeof(float) * 3 * vertex_data.count;
      vertex_data.data = new char[vertex_data.data_size];
      memcpy(vertex_data.data, mesh->mVertices, vertex_data.data_size);
    }
    if (mesh->HasFaces()) {
      eastl::vector<uint32_t> indices;
      for (uint32_t j = 0; j < mesh->mNumFaces; j++) {
        indices.push_back(mesh->mFaces[j].mIndices[0]);
        indices.push_back(mesh->mFaces[j].mIndices[1]);
        indices.push_back(mesh->mFaces[j].mIndices[2]);
      }
      Mesh::MeshData& index_data = new_mesh->GetIndexData();
      index_data.count = indices.size();
      index_data.data_size = sizeof(uint32_t) * index_data.count;
      index_data.data = new char[index_data.data_size];
      memcpy(index_data.data, indices.data(), index_data.data_size);
    }
    if (mesh->HasNormals()) {
      Mesh::MeshData& normal_data = new_mesh->GetNormalData();
      normal_data.count = mesh->mNumVertices;
      normal_data.data_size = sizeof(float) * 3 * normal_data.count;
      normal_data.data = new char[normal_data.data_size];
      memcpy(normal_data.data, mesh->mNormals, normal_data.data_size);
    }
    // TODO: SUPPORT MUTLIPLE UV COORDS ?
    if (mesh->HasTextureCoords(0)) {
      Mesh::MeshData& uv_data = new_mesh->GetUVData();
      uv_data.count = mesh->mNumVertices;
      uv_data.data_size = sizeof(float) * 3 * uv_data.count;
      uv_data.data = new char[uv_data.data_size];
      memcpy(uv_data.data, mesh->mTextureCoords[0], uv_data.data_size);
    }

    meshes_.push_back(new_mesh);
  }

  // Why would one texture type have multiple textures...
  // TODO: investigate
  for (uint32_t i = 0; i < scene->mNumMaterials; i++) {
    auto& material = scene->mMaterials[i];
    Material new_mat = Material();
    aiString filepath;

    if (material->GetTextureCount(aiTextureType_BASE_COLOR)) {
      material->GetTexture(aiTextureType_BASE_COLOR, 0, &filepath);
      new_mat.GetBaseColorTextureInfo().filepath = filepath.C_Str();
    }
    if (material->GetTextureCount(aiTextureType_NORMAL_CAMERA)) {
      material->GetTexture(aiTextureType_NORMAL_CAMERA, 0, &filepath);
      new_mat.GetNormalTextureInfo().filepath = filepath.C_Str();
    }
    if (material->GetTextureCount(aiTextureType_EMISSION_COLOR)) {
      material->GetTexture(aiTextureType_EMISSION_COLOR, 0, &filepath);
      new_mat.GetEmissionTextureInfo().filepath = filepath.C_Str();
    }
    if (material->GetTextureCount(aiTextureType_METALNESS)) {
      material->GetTexture(aiTextureType_METALNESS, 0, &filepath);
      new_mat.GetMetalnessTextureInfo().filepath = filepath.C_Str();
    }
    if (material->GetTextureCount(aiTextureType_DIFFUSE_ROUGHNESS)) {
      material->GetTexture(aiTextureType_DIFFUSE_ROUGHNESS, 0, &filepath);
      new_mat.GetDiffuseRoughnessTextureInfo().filepath = filepath.C_Str();
    }
    if (material->GetTextureCount(aiTextureType_AMBIENT_OCCLUSION)) {
      material->GetTexture(aiTextureType_AMBIENT_OCCLUSION, 0, &filepath);
      new_mat.GetAmbientOcclusionTextureInfo().filepath = filepath.C_Str();
    }
    materials_.push_back(new_mat);
  }
}

void blu::core::components::Model::Delete() {
  for (auto& mesh : meshes_) {
    delete mesh;
  }
}