#ifndef IMGUISTAGE_H
#define IMGUISTAGE_H

#include <imgui.h>

#include "../../External/Window.h"
#include "../Vulkan/Image.h"
#include "../Vulkan/Stage.h"
#include "../includes/imgui_impl_glfw.h"
#include "../includes/imgui_impl_vulkan.h"

namespace blu::core::rendering::stage {
class ImGuiStage : protected Stage {
 public:
  ImGuiStage(Instance*, Device*, Window*, uint32_t frame_count);
  ~ImGuiStage();

  void Start(VkCommandBuffer buf, Image* dst, uint32_t width, uint32_t height);
  void End(VkCommandBuffer buf);
};
}  // namespace blu::core::rendering
#endif