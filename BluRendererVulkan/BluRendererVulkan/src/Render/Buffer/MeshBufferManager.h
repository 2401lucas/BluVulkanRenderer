#pragma once
#include "../Device/Device.h"
#include "BufferAllocator.h"
#include "../../Engine/Mesh/MeshUtils.h"

class MeshBufferManager {
 public:
  MeshBufferManager(Device*);
  void cleanup(Device*);
  void registerMesh(const MeshData&);
  void rebuildBuffers(Device* deviceInfo, CommandPool* commandPool);
  void bindBuffers(const VkCommandBuffer&);
  void drawIndexedIndirect(const VkCommandBuffer&);

 private:
  std::vector<VkDrawIndexedIndirectCommand> indirectCommands;
  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;
  Buffer* vertexBuffer = nullptr;
  Buffer* indexBuffer = nullptr;
  Buffer* indirectCommandsBuffer = nullptr;
};