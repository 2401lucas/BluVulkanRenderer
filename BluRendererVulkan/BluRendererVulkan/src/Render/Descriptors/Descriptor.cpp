#include "Descriptor.h"
#include <stdexcept>

Descriptor::Descriptor(Device* deviceInfo, std::vector<VkDescriptorSetLayoutBinding> bindings) {
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(deviceInfo->getLogicalDevice(), &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }
}

void Descriptor::cleanup(Device* deviceInfo)
{
    vkDestroyDescriptorSetLayout(deviceInfo->getLogicalDevice(), descriptorSetLayout, nullptr);
}

const VkDescriptorSetLayout* Descriptor::getLayout()
{
    return &descriptorSetLayout;
}
