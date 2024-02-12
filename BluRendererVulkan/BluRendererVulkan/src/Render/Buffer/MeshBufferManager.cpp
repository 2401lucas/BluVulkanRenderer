#include "MeshBufferManager.h"

MeshBufferManager::MeshBufferManager(Device* deviceInfo) {}

void MeshBufferManager::cleanup(Device* deviceInfo) {}

void MeshBufferManager::registerMesh(const MeshData& meshData) {
  VkDrawIndexedIndirectCommand cmd{};
  cmd.firstInstance = indirectCommands.size();
  cmd.instanceCount = 1;
  cmd.firstIndex = indices.size();
  cmd.indexCount = meshData.indices.size();
  indirectCommands.push_back(cmd);

  vertices.insert(std::end(vertices), std::begin(meshData.vertices),
                  std::end(meshData.vertices));
  indices.insert(std::end(indices), std::begin(meshData.indices),
                 std::end(meshData.indices));
}

void MeshBufferManager::rebuildBuffers(Device* deviceInfo,
                                       CommandPool* commandPool) {
  if (vertexBuffer != nullptr) {
    vertexBuffer->freeBuffer(deviceInfo);
    delete vertexBuffer;
  }
  if (indexBuffer != nullptr) {
    indexBuffer->freeBuffer(deviceInfo);
    delete indexBuffer;
  }
  if (indirectCommandsBuffer != nullptr) {
    indirectCommandsBuffer->freeBuffer(deviceInfo);
    delete indirectCommandsBuffer;
  }

  VkDeviceSize vertexBufferSize = sizeof(Vertex) * vertices.size();
  Buffer* vertexStagingBuffer =
      new Buffer(deviceInfo, vertexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  vertexStagingBuffer->copyData(deviceInfo, vertices.data(), 0,
                                vertexBufferSize, 0);
  vertexBuffer = new Buffer(
      deviceInfo, vertexBufferSize,
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  vertexBuffer->copyBuffer(deviceInfo, commandPool, vertexStagingBuffer,
                           vertexBufferSize, 0);
  vertexStagingBuffer->freeBuffer(deviceInfo);
  delete vertexStagingBuffer;

  VkDeviceSize indicesBufferSize = sizeof(uint32_t) * indices.size();
  Buffer* indexStagingBuffer = new Buffer(
      deviceInfo, indicesBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  indexStagingBuffer->copyData(deviceInfo, indices.data(), 0, indicesBufferSize,
                               0);
  indexBuffer = new Buffer(
      deviceInfo, indicesBufferSize,
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  indexBuffer->copyBuffer(deviceInfo, commandPool, indexStagingBuffer,
                          indicesBufferSize, 0);
  indexStagingBuffer->freeBuffer(deviceInfo);
  delete indexStagingBuffer;

  VkDeviceSize indirectCommandsBufferSize =
      sizeof(VkDrawIndexedIndirectCommand) * indirectCommands.size();

  Buffer* indirectCommandsStagingBuffer = new Buffer(
      deviceInfo, indirectCommandsBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  indirectCommandsStagingBuffer->copyData(deviceInfo, indirectCommands.data(),
                                          0, indirectCommandsBufferSize, 0);
  indirectCommandsBuffer = new Buffer(
      deviceInfo, indirectCommandsBufferSize,
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  indirectCommandsBuffer->copyBuffer(deviceInfo, commandPool,
                                     indirectCommandsStagingBuffer,
                                     indirectCommandsBufferSize, 0);
  indirectCommandsStagingBuffer->freeBuffer(deviceInfo);
  delete indirectCommandsStagingBuffer;
}

void MeshBufferManager::bindBuffers(const VkCommandBuffer& commandBuffer) {
  VkDeviceSize offsets[] = {0};
  vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer->getBuffer(),
                         offsets);
  vkCmdBindIndexBuffer(commandBuffer, indexBuffer->getBuffer(), 0,
                       VK_INDEX_TYPE_UINT32);
}

void MeshBufferManager::drawIndexedIndirect(
    const VkCommandBuffer& commandBuffer) {
  vkCmdDrawIndexedIndirect(commandBuffer, indirectCommandsBuffer->getBuffer(),
                           0, indirectCommands.size(),
                           sizeof(VkDrawIndexedIndirectCommand));
}