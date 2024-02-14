#pragma once
#include "../../Engine/Mesh/MeshUtils.h"
#include "../Device/Device.h"
#include "BufferAllocator.h"

class MeshBufferManager {
 public:
  MeshBufferManager(Device*);
  void cleanup(Device*);
  void registerMesh(const MeshData&);
  void rebuildBuffers(Device*, CommandPool*);
  void updateInstanceBuffers(Device*, CommandPool*, std::vector<InstanceData>);
  void bindBuffers(const VkCommandBuffer&);
  void drawIndexedIndirect(const VkCommandBuffer&);

 private:
  std::unordered_map<std::string, uint32_t> indirectCommandIndices;
  std::vector<VkDrawIndexedIndirectCommand> indirectCommands;

  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;
  Buffer* vertexBuffer = nullptr;
  Buffer* indexBuffer = nullptr;
  Buffer* indirectCommandsBuffer = nullptr;
  Buffer* instanceBuffer = nullptr;
};