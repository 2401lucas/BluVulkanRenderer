#include "DescriptorSetManager.h"
#include <stdexcept>
#include "../Descriptors/Types/UBO/UBO.h"
#include "DescriptorUtils.h"


DescriptorSetManager::DescriptorSetManager(Device* deviceInfo, const std::vector<VkDescriptorSetLayout>& descriptorLayouts, ModelBufferManager* modelBufferManager, TextureManager* textureManager)
{
    std::vector<VkDescriptorPoolSize> globalPoolSizes{2, VkDescriptorPoolSize()};
    globalPoolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    globalPoolSizes[0].descriptorCount = static_cast<uint32_t>(RenderConst::MAX_FRAMES_IN_FLIGHT);
    globalPoolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    globalPoolSizes[1].descriptorCount = static_cast<uint32_t>(RenderConst::MAX_FRAMES_IN_FLIGHT);

    std::vector<VkDescriptorPoolSize> matPoolSizes{ 2, VkDescriptorPoolSize() };
    matPoolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    matPoolSizes[0].descriptorCount = static_cast<uint32_t>(RenderConst::MAX_FRAMES_IN_FLIGHT);
    matPoolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    matPoolSizes[1].descriptorCount = static_cast<uint32_t>(RenderConst::MAX_FRAMES_IN_FLIGHT);

    globalDescriptorPool = new DescriptorPool(deviceInfo, globalPoolSizes, RenderConst::MAX_FRAMES_IN_FLIGHT, 0);
    matDescriptorPool = new DescriptorPool(deviceInfo, matPoolSizes, RenderConst::MAX_FRAMES_IN_FLIGHT, 0);
    createDescriptorSets(deviceInfo, descriptorLayouts, modelBufferManager, textureManager);
}

void DescriptorSetManager::cleanup(Device* deviceInfo)
{
    globalDescriptorPool->cleanup(deviceInfo);
    delete globalDescriptorPool;
    matDescriptorPool->cleanup(deviceInfo);
    delete matDescriptorPool;
}

//Descriptor Set 0 Global Resources and bound once per frame.
//  Vertex Data:
//      Contains MVP matrix data, where M is an array of all the models matrix data up to a max of MAX_MODELS
//  Fragment Data: 
//      Contains all lighting data
//Descriptor Set 1 Material Data
//  Fragment Data: 
//      An array of all textures up to a max of MAX_TEXTURES, updated only when textures are added (TODO Investigate: Possibly when removed too, but this shouldn't happen often, or at all)
void DescriptorSetManager::createDescriptorSets(Device* deviceInfo, const std::vector<VkDescriptorSetLayout>& descriptorLayouts, ModelBufferManager* modelBufferManager, TextureManager* textureManager)
{
    std::vector<VkDescriptorSetLayout> globalLayout(RenderConst::MAX_FRAMES_IN_FLIGHT, descriptorLayouts[0]);
    VkDescriptorSetAllocateInfo globalAllocInfo{};
    globalAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    globalAllocInfo.descriptorPool = globalDescriptorPool->getDescriptorPool();
    globalAllocInfo.descriptorSetCount = static_cast<uint32_t>(RenderConst::MAX_FRAMES_IN_FLIGHT);
    globalAllocInfo.pSetLayouts = globalLayout.data();

    globalDescriptorSets.resize(RenderConst::MAX_FRAMES_IN_FLIGHT);
    if (vkAllocateDescriptorSets(deviceInfo->getLogicalDevice(), &globalAllocInfo, globalDescriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }

    std::vector<VkDescriptorSetLayout> materialLayouts(RenderConst::MAX_FRAMES_IN_FLIGHT, descriptorLayouts[1]);
    VkDescriptorSetAllocateInfo materialAllocInfo{};
    materialAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    materialAllocInfo.descriptorPool = matDescriptorPool->getDescriptorPool();
    materialAllocInfo.descriptorSetCount = static_cast<uint32_t>(RenderConst::MAX_FRAMES_IN_FLIGHT);
    materialAllocInfo.pSetLayouts = materialLayouts.data(); 
   

    materialDescriptorSets.resize(RenderConst::MAX_FRAMES_IN_FLIGHT);
    if (vkAllocateDescriptorSets(deviceInfo->getLogicalDevice(), &materialAllocInfo, materialDescriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }
    for (uint32_t i = 0; i < RenderConst::MAX_FRAMES_IN_FLIGHT; i++) {
        std::vector<VkWriteDescriptorSet> descriptorWrites;

        //Global Descriptor Set
        VkDescriptorBufferInfo gpuCameraBufferInfo{};
        gpuCameraBufferInfo.buffer = modelBufferManager->getMappedBufferManager(0)->getUniformBuffer(i)->getBuffer();
        gpuCameraBufferInfo.offset = 0;
        gpuCameraBufferInfo.range = sizeof(GPUCameraData);

        VkDescriptorBufferInfo gpuSceneBufferInfo{};
        gpuSceneBufferInfo.buffer = modelBufferManager->getMappedBufferManager(1)->getUniformBuffer(i)->getBuffer();
        gpuSceneBufferInfo.offset = 0;
        gpuSceneBufferInfo.range = sizeof(GPUSceneData);

        descriptorWrites.push_back(DescriptorUtils::createBufferDescriptorWriteSet(globalDescriptorSets[i], 0, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, &gpuCameraBufferInfo));
        descriptorWrites.push_back(DescriptorUtils::createBufferDescriptorWriteSet(globalDescriptorSets[i], 1, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, &gpuSceneBufferInfo));

        //Material Descriptor Set
        std::vector<VkDescriptorImageInfo> textureImageDescriptorInfo;
        auto& textures = textureManager->getTextures();
        for (uint32_t matIndex = 0; matIndex < textures.size(); matIndex++) {
            VkDescriptorImageInfo texImageInfo{};
            texImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            texImageInfo.imageView = textures[matIndex].texture->getImageView();
            texImageInfo.sampler = textures[matIndex].texture->getImageSampler();
            textureImageDescriptorInfo.push_back(texImageInfo);
            VkDescriptorImageInfo diffImageInfo{};
            diffImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            diffImageInfo.imageView = textures[matIndex].diffuse->getImageView();
            diffImageInfo.sampler = textures[matIndex].diffuse->getImageSampler();
            textureImageDescriptorInfo.push_back(diffImageInfo);
            VkDescriptorImageInfo specImageInfo{};
            specImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            specImageInfo.imageView = textures[matIndex].specular->getImageView();
            specImageInfo.sampler = textures[matIndex].specular->getImageSampler();
            textureImageDescriptorInfo.push_back(specImageInfo);
        }

        descriptorWrites.push_back(DescriptorUtils::createImageDescriptorWriteSet(materialDescriptorSets[i], 0, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, textureImageDescriptorInfo.size(), textureImageDescriptorInfo.data()));
        
        /*VkDescriptorBufferInfo materialBufferInfo{};
        materialBufferInfo.buffer = modelManager->getMappedBufferManager(2)->getUniformBuffer(i)->getBuffer();
        materialBufferInfo.offset = 0;
        materialBufferInfo.range = sizeof(GPUMaterialData);

        descriptorWrites.push_back(DescriptorUtils::createBufferDescriptorWriteSet(materialDescriptorSets[i], 1, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, &materialBufferInfo));*/

        vkUpdateDescriptorSets(deviceInfo->getLogicalDevice(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }
}

VkDescriptorSet* DescriptorSetManager::getGlobalDescriptorSet(uint32_t index)
{
    return &globalDescriptorSets[index];
}

VkDescriptorSet* DescriptorSetManager::getMaterialDescriptorSet(uint32_t index)
{
    return &materialDescriptorSets[index];
}
