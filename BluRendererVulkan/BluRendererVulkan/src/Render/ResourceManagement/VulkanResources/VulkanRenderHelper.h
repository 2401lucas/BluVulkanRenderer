#pragma once

#include "VulkanRenderTarget.h"

namespace vks {
namespace rendering {
vks::VulkanRenderTarget* createDepthRenderTarget(
    vks::VulkanDevice* device, VkFormat depthFormat, VkFilter samplerFilter, uint32_t imageCount,
    float depthMapWidth, float depthMapHeight, std::string vertexShaderPath);
vks::VulkanRenderTarget* createColorDepthRenderTarget(
    VulkanDevice* device, VkFormat colorFormat, VkFormat depthFormat,
    uint32_t imageCount, float width, float height,
    std::string vertexShaderPath, std::string fragmentShaderPath);
void recreateColorDepthRenderTargetResources(vks::VulkanRenderTarget* newTarget,
                                             uint32_t imageCount,
                                             uint32_t width, uint32_t height);
void recreateColorRenderTargetResources(vks::VulkanRenderTarget* newTarget,
                                             uint32_t imageCount,
                                             uint32_t width, uint32_t height);
void recreateDepthRenderTargetResources(vks::VulkanRenderTarget* newTarget,
                                             uint32_t imageCount,
                                             uint32_t width, uint32_t height);

void createImageFromBuffer(vks::VulkanRenderTarget* target, void** imageData,
                           float dataSize, uint32_t width, uint32_t height,
                           VkFormat imageFormat, VkQueue copyQueue);
}  // namespace rendering
}  // namespace vks