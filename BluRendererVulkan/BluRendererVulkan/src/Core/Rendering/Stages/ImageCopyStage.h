#ifndef IMAGECOPYSTAGE_H
#define IMAGECOPYSTAGE_H

#include "../Vulkan/Stage.h"

namespace blu::core::rendering {
class ImageCopyStage : protected Stage {
 public:
  ImageCopyStage(Device* device, VmaAllocator allocator);
  ~ImageCopyStage();

  void Run(VkCommandBuffer buf, uint32_t frame_index, VkImage src_img,
           VkImageLayout src_image_layout, VkImage dst_img,
           VkImageLayout dst_img_layout, uint32_t width, uint32_t height);
};
}  // namespace blu::core::rendering
#endif