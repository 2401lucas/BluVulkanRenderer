#include "DescriptorSetManager.h"
#include <stdexcept>
#include "../Descriptors/Types/UBO/UBO.h"


DescriptorSetManager::DescriptorSetManager(Device* deviceInfo, Descriptor* descriptorSetLayout, ModelManager* modelManager)
{
	descriptorPool = new DescriptorPool(deviceInfo);
    createDescriptorSets(deviceInfo, descriptorSetLayout, modelManager);
}

void DescriptorSetManager::cleanup(Device* deviceInfo)
{
    descriptorPool->cleanup(deviceInfo);
    delete descriptorPool;
}

//TODO: 10 MAKE COMPATIBLE WITH STD::VECTOR<MODEL>
void DescriptorSetManager::createDescriptorSets(Device* deviceInfo, Descriptor* descriptorSetLayout, ModelManager* modelManager)
{
    std::vector<VkDescriptorSetLayout> layouts(RenderConst::MAX_FRAMES_IN_FLIGHT, *descriptorSetLayout->getLayout());
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool->getDescriptorPool();
    allocInfo.descriptorSetCount = static_cast<uint32_t>(RenderConst::MAX_FRAMES_IN_FLIGHT);
    allocInfo.pSetLayouts = layouts.data();

    descriptorSets.resize(RenderConst::MAX_FRAMES_IN_FLIGHT);
    if (vkAllocateDescriptorSets(deviceInfo->getLogicalDevice(), &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }

    for (uint32_t i = 0; i < RenderConst::MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = modelManager->getUniformBuffer(i)->getBuffer();
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = modelManager->getModel(0)->getTexture()->getImageView();
        imageInfo.sampler = modelManager->getModel(0)->getTexture()->getImageSampler();

        std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = descriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfo;

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = descriptorSets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(deviceInfo->getLogicalDevice(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }
}

VkDescriptorSet* DescriptorSetManager::getDescriptorSet(uint32_t index)
{
    return &descriptorSets[index];
}