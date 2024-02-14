#pragma once
#include "../Command/CommandPool.h"
#include "../Device/Device.h"

class Buffer {
 public:
  Buffer(Device* deviceInfo, const VkDeviceSize& bufferSize,
         const VkBufferUsageFlags& usage,
         const VkMemoryPropertyFlags& properties);

  void copyBuffer(Device* deviceInfo, CommandPool* commandPool,
                  Buffer* srcBuffer, const VkDeviceSize& size,
                  const VkDeviceSize& dstOffset);
  void copyData(Device* deviceInfo, const void* src, const VkDeviceSize& offset,
                const VkDeviceSize& deviceSize, const VkMemoryMapFlags& flags);
  void flush(Device* device, VkDeviceSize& size, VkDeviceSize offset);
  void freeBuffer(Device* deviceInfo);
  VkBuffer& getBuffer();
  VkDeviceMemory getBufferMemory();

 private:
  VkBuffer buffer;
  VkDeviceMemory bufferMemory;
};