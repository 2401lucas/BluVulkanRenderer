#ifndef STAGE_H
#define STAGE_H

#include <EASTL/vector.h>
#include <vulkan/vulkan.h>

#include "../BluRendererConsts.h"
#include "../Vulkan/Device.h"
#include "../Vulkan/Tools.h"

namespace blu::core::rendering {
class Stage {
 protected:
  Stage(Device* device, VmaAllocator allocator);

 public:
  ~Stage();

 protected:
  Device* device_;
  VmaAllocator allocator_;
};
}  // namespace blu::core::rendering
#endif