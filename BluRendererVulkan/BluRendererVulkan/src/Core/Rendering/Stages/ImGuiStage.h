#ifndef IMGUISTAGE_H
#define IMGUISTAGE_H

#include <imgui.h>

#include "../../External/Window.h"
#include "../Vulkan/Stage.h"
#include "../includes/imgui_impl_glfw.h"
#include "../includes/imgui_impl_vulkan.h"

namespace blu::core::rendering {
class ImGuiStage : protected Stage {
 public:
  ImGuiStage(Instance*, Device*, Window*, uint32_t frame_count,
             eastl::vector<VkCommandPool> pools);
  ~ImGuiStage();

  void Resize();
  void Run(uint32_t frame_index, VkSemaphore wait_semaphore,
           uint64_t wait_value, VkPipelineStageFlags wait_flag,
           VkSemaphore signal_semaphore, uint64_t signal_value, VkFence fence);

 private:
  uint32_t width_;
  uint32_t height;
};
}  // namespace blu::core::rendering
#endif