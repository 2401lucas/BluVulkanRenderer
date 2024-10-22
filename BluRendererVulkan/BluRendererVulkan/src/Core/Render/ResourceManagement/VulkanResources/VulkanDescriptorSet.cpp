#include "VulkanDescriptorSet.hpp"

namespace core_internal::rendering {
VulkanDescriptorSet::VulkanDescriptorSet(VulkanDevice* vulkanDevice)
    : vulkanDevice(vulkanDevice) {}

VulkanDescriptorSet::~VulkanDescriptorSet() {}

void VulkanDescriptorSet::addBinding(uint32_t binding,
                                     VkDescriptorType descriptorType,
                                     uint32_t descriptorCount,
                                     VkPipelineStageFlags stageFlags,
                                     const VkSampler* pImmutableSamplers) {
  bindings.push_back({binding, descriptorType, descriptorCount, stageFlags,
                      pImmutableSamplers});
}

void VulkanDescriptorSet::initLayout() {
  assert(layout == VK_NULL_HANDLE);

  VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCI{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .bindingCount = static_cast<uint32_t>(bindings.size()),
      .pBindings = bindings.data(),
  };

  VK_CHECK_RESULT(vkCreateDescriptorSetLayout(vulkanDevice->operator VkDevice(),
                                              &descriptorSetLayoutCI, nullptr,
                                              &layout));
}

void VulkanDescriptorSet::allocateDescriptorSets(VkDescriptorPool* pools,
                                                 uint32_t poolCount) {
  setCount = poolCount;
  sets = new VkDescriptorSet[setCount];
  for (size_t i = 0; i < poolCount; i++) {
    VkDescriptorSetAllocateInfo allocInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = pools[i],
        .descriptorSetCount = 1,
        .pSetLayouts = &layout,
    };

    vkAllocateDescriptorSets(vulkanDevice->operator VkDevice(), &allocInfo,
                             &sets[i]);
  }
}

VkDescriptorSet VulkanDescriptorSet::getSet(uint32_t index) {
  assert(index < setCount);

  return sets[index];
}

VkWriteDescriptorSet core_internal::rendering::VulkanDescriptorSet::makeWrite(
    VkDescriptorSet dstSet, uint32_t dstBinding,
    const VkDescriptorImageInfo* pImageInfo, uint32_t arrayElement) const {
  VkWriteDescriptorSet writeSet = makeWrite(dstSet, dstBinding, arrayElement);
  assert(writeSet.descriptorType == VK_DESCRIPTOR_TYPE_SAMPLER ||
         writeSet.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ||
         writeSet.descriptorType == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE ||
         writeSet.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE ||
         writeSet.descriptorType == VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT);

  writeSet.pImageInfo = pImageInfo;
  return writeSet;
}

VkWriteDescriptorSet core_internal::rendering::VulkanDescriptorSet::makeWrite(
    VkDescriptorSet dstSet, uint32_t dstBinding,
    const VkDescriptorBufferInfo* pBufferInfo, uint32_t arrayElement) const {
  VkWriteDescriptorSet writeSet = makeWrite(dstSet, dstBinding, arrayElement);
  assert(writeSet.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ||
         writeSet.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC ||
         writeSet.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ||
         writeSet.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC);

  writeSet.pBufferInfo = pBufferInfo;
  return writeSet;
}

VkWriteDescriptorSet core_internal::rendering::VulkanDescriptorSet::makeWrite(
    VkDescriptorSet dstSet, uint32_t dstBinding,
    const VkWriteDescriptorSetAccelerationStructureKHR* pAccel,
    uint32_t arrayElement) const {
  VkWriteDescriptorSet writeSet = makeWrite(dstSet, dstBinding, arrayElement);
  assert(writeSet.descriptorType ==
         VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR);

  writeSet.pNext = pAccel;
  return writeSet;
}

VkWriteDescriptorSet core_internal::rendering::VulkanDescriptorSet::makeWrite(
    VkDescriptorSet dstSet, uint32_t dstBinding, uint32_t arrayElement) const {
  VkWriteDescriptorSet writeSet = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
  writeSet.descriptorType = VK_DESCRIPTOR_TYPE_MAX_ENUM;
  for (size_t i = 0; i < bindings.size(); i++) {
    if (bindings[i].binding == dstBinding) {
      writeSet.descriptorCount = 1;
      writeSet.descriptorType = bindings[i].descriptorType;
      writeSet.dstBinding = dstBinding;
      writeSet.dstSet = dstSet;
      writeSet.dstArrayElement = arrayElement;
      return writeSet;
    }
  }
  assert(0 && "binding not found");
  return writeSet;
}
}  // namespace core_internal::rendering