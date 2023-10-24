#include "DescriptorSetManager.h"
#include <stdexcept>
#include "../Descriptors/Types/UBO/UBO.h"
#include "DescriptorUtils.h"


DescriptorSetManager::DescriptorSetManager(Device* deviceInfo, Descriptor* descriptorSetLayout, ModelManager* modelManager)
{
    std::vector<VkDescriptorPoolSize> poolSizes{3, VkDescriptorPoolSize()};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(RenderConst::MAX_FRAMES_IN_FLIGHT);
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[1].descriptorCount = static_cast<uint32_t>(RenderConst::MAX_FRAMES_IN_FLIGHT);
    poolSizes[2].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[2].descriptorCount = static_cast<uint32_t>(RenderConst::MAX_FRAMES_IN_FLIGHT);

    descriptorPool = new DescriptorPool(deviceInfo, poolSizes, RenderConst::MAX_FRAMES_IN_FLIGHT, 0);
    createDescriptorSets(deviceInfo, descriptorSetLayout, modelManager);
}

void DescriptorSetManager::cleanup(Device* deviceInfo)
{
    descriptorPool->cleanup(deviceInfo);
    delete descriptorPool;
}

//TODO: 10 MAKE COMPATIBLE WITH STD::VECTOR<MODEL>
//Descriptor Set 0 Global Resources and bound once per frame.
//Descriptor Set 1 Per-pass Resources, and bound once per pass
//Descriptor Set 2 Material Resources
//Descriptor Set 3 Per-Object Resources
void DescriptorSetManager::createDescriptorSets(Device* deviceInfo, Descriptor* descriptorSetLayout, ModelManager* modelManager)
{
    std::vector<VkDescriptorSetLayout> layouts(RenderConst::MAX_FRAMES_IN_FLIGHT, descriptorSetLayout->getLayout());
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
        std::vector<VkWriteDescriptorSet> descriptorWrites;
        
        //Global Descriptor Set
        VkDescriptorBufferInfo gpuCameraBufferInfo{};
        gpuCameraBufferInfo.buffer = modelManager->getMappedBufferManager(0)->getUniformBuffer(i)->getBuffer();
        gpuCameraBufferInfo.offset = 0;
        gpuCameraBufferInfo.range = sizeof(GPUCameraData);

        VkDescriptorBufferInfo gpuSceneBufferInfo{};
        gpuSceneBufferInfo.buffer = modelManager->getMappedBufferManager(1)->getUniformBuffer(i)->getBuffer();
        gpuSceneBufferInfo.offset = DescriptorUtils::padUniformBufferSize(sizeof(GPUSceneData), deviceInfo->getGPUProperties().limits.minUniformBufferOffsetAlignment) * i;
        gpuSceneBufferInfo.range = sizeof(GPUSceneData);

        descriptorWrites.push_back(DescriptorUtils::createBufferDescriptorWriteSet(descriptorSets[i], 0, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, &gpuCameraBufferInfo));
        descriptorWrites.push_back(DescriptorUtils::createBufferDescriptorWriteSet(descriptorSets[i], 1, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, &gpuSceneBufferInfo));
        //PerPass Descriptor Set

        //Material Descriptor Set
        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = modelManager->getModel(0)->getTexture()->getImageView();
        imageInfo.sampler = modelManager->getModel(0)->getTexture()->getImageSampler();

        descriptorWrites.push_back(DescriptorUtils::createImageDescriptorWriteSet(descriptorSets[i], 2, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &imageInfo));

        //Per-Object Descriptor Set (Transform Data)

        vkUpdateDescriptorSets(deviceInfo->getLogicalDevice(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }
}

VkDescriptorSet* DescriptorSetManager::getDescriptorSet(uint32_t index)
{
    return &descriptorSets[index];
}

VkDescriptorSet* DescriptorSetManager::getGlobalDescriptorSet()
{
    return &globalDescriptorSet;
}
