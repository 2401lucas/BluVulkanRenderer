#include "VulkanRenderpass.hpp"

namespace core_internal::rendering {
VulkanRenderpass::VulkanRenderpass(
    const core_internal::rendering::Image** images, const uint32_t& imgCount) {
  attachments = new VkRenderingAttachmentInfoKHR[imgCount];

  VkClearValue clearValues[2] = {
      {.color = {1.0f, 0.0f, 0.0f, 1.0f}},
      {.depthStencil = {1.0f, 0}},
  };

  for (uint32_t i = 0; i < imgCount; i++) {
    attachments[i] = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView = images[i]->view,
        .imageLayout = images[i]->imageLayout,
        .resolveMode = VK_RESOLVE_MODE_NONE,
        //.resolveImageView = VK_NULL_HANDLE,
        //.resolveImageLayout = VK_NULL_HANDLE,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue = {},
    };
  }
}  // namespace core_internal::rendering

VulkanRenderpass::~VulkanRenderpass() {}

}  // namespace core_internal::rendering