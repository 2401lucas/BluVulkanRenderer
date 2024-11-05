struct BufferInfo {
  VkDeviceAddress address;
  VkDeviceSize offset;
  VkDeviceSize size;
};

layout (set = 0, binding = 0) uniform Buffers{
    BufferInfo buffers[];
}