#include "MeshBufferManager.h"

MeshBufferManager::MeshBufferManager(Device* deviceInfo) {}

void MeshBufferManager::cleanup(Device* deviceInfo) {}

void MeshBufferManager::registerMesh(const MeshData& meshData) {
  if (indirectCommands.count(meshData.meshPath) == 0) {
    VkDrawIndexedIndirectCommand cmd{};
    cmd.firstInstance = indirectCommands.size();
    cmd.instanceCount = 1;
    cmd.firstIndex = indices.size();
    cmd.indexCount = meshData.indices.size();
    indirectCommands[meshData.meshPath] = cmd;
    vertices.insert(std::end(vertices), std::begin(meshData.vertices),
                    std::end(meshData.vertices));
    indices.insert(std::end(indices), std::begin(meshData.indices),
                   std::end(meshData.indices));
  } else {
    indirectCommands[meshData.meshPath].instanceCount++;
  }
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

  // Yeah, this is not ideal. I should probably write a custom map that allows
  // me to access all values data but that is for another day.
  // Even in worst case Scenario, this is only running when buffers are rebuild
  // and it should not be happening often
  std::vector<VkDrawIndexedIndirectCommand> cmds;
  for (auto& [key, value] : indirectCommands) {
    cmds.push_back(value);
  }

  VkDeviceSize indirectCommandsBufferSize =
      sizeof(VkDrawIndexedIndirectCommand) * cmds.size();

  Buffer* indirectCommandsStagingBuffer = new Buffer(
      deviceInfo, indirectCommandsBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  indirectCommandsStagingBuffer->copyData(deviceInfo, cmds.data(), 0,
                                          indirectCommandsBufferSize, 0);
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

void MeshBufferManager::updateInstanceBuffers(
    Device* deviceInfo, CommandPool* commandPool, std::vector<InstanceData> instanceData) {
  if (instanceBuffer != nullptr) {
    instanceBuffer->freeBuffer(deviceInfo);
    delete instanceBuffer;
  }

  VkDeviceSize instanceBufferSize = sizeof(InstanceData) * instanceData.size();
  Buffer* instanceStagingBuffer = new Buffer(
      deviceInfo, instanceBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  instanceStagingBuffer->copyData(deviceInfo, instanceData.data(), 0,
                                  instanceBufferSize, 0);
  instanceBuffer = new Buffer(
      deviceInfo, instanceBufferSize,
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  instanceBuffer->copyBuffer(deviceInfo, commandPool, instanceStagingBuffer,
                           instanceBufferSize, 0);
  instanceStagingBuffer->freeBuffer(deviceInfo);
  delete instanceStagingBuffer;
}

void MeshBufferManager::bindBuffers(const VkCommandBuffer& commandBuffer) {
  VkDeviceSize offsets[] = {0};
  vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer->getBuffer(),
                         offsets);
  vkCmdBindVertexBuffers(commandBuffer, 1, 1, &instanceBuffer->getBuffer(),
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