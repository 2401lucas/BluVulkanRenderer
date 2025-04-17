#include "ImageCopyStage.h"

#include "../Vulkan/Image.h"

namespace blu::core::rendering::stage {
ImageCopyStage::ImageCopyStage(Device* device, VmaAllocator allocator)
    : Stage(device, allocator) {}

ImageCopyStage::~ImageCopyStage() {}

void ImageCopyStage::Run(VkCommandBuffer buf, VkImage src_img,
                         VkImageLayout src_image_layout, VkImage dst_img,
                         VkImageLayout dst_img_layout, uint32_t width,
                         uint32_t height) {
  VkImageSubresourceRange range{
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .baseMipLevel = 0,
      .levelCount = 1,
      .baseArrayLayer = 0,
      .layerCount = 1,
  };

  blu::core::Image::ImageLayoutTransition(buf, src_img, src_image_layout,
                                          VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                          range);

  blu::core::Image::ImageLayoutTransition(
      buf, dst_img, VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, range);

  VkImageCopy img_cpy{
      .srcSubresource{
          .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
          .mipLevel = 0,
          .baseArrayLayer = 0,
          .layerCount = 1,
      },
      .srcOffset{0, 0, 0},
      .dstSubresource{
          .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
          .mipLevel = 0,
          .baseArrayLayer = 0,
          .layerCount = 1,
      },
      .dstOffset{0, 0, 0},
      .extent{.width = width, .height = height, .depth = 1},
  };

  vkCmdCopyImage(buf, src_img, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dst_img,
                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &img_cpy);

  blu::core::Image::ImageLayoutTransition(buf, dst_img,
                                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                          dst_img_layout, range);
}
}  // namespace blu::core::rendering::stage