#include "Stage.h"

namespace blu::core::rendering {
Stage::Stage(Device* device, VmaAllocator allocator) {
  device_ = device;
  allocator_ = allocator;
}

Stage::~Stage() {}
}  // namespace blu::core::rendering
