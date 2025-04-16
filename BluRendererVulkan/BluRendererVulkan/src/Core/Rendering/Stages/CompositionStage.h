#ifndef COMPOSITIONSTAGE_H
#define COMPOSITIONSTAGE_H

#include "../Vulkan/Stage.h"

namespace blu::core::rendering {
class CompositionStage : protected Stage {
 public:
  CompositionStage();
  ~CompositionStage();


};
}  // namespace blu::core::rendering
#endif