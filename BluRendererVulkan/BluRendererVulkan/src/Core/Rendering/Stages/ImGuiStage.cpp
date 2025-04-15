#include "ImGuiStage.h"

 static void CheckVkResult(VkResult err) {
   if (err == 0) return;
   fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
   if (err < 0) abort();
 }

namespace blu::core::rendering {
ImGuiStage::ImGuiStage(Instance* instance, Device* device, Window* window,
                       uint32_t frame_count, eastl::vector<VkCommandPool> pools)
    : Stage(device, nullptr, nullptr, pools) {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |=
      ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
  io.ConfigFlags |=
      ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls

  ImGui_ImplVulkan_InitInfo init_info = {};
  init_info.Instance = instance->Get();
  init_info.PhysicalDevice = device_->GetPhysicalDevice();
  init_info.Device = device_->GetLogicalDevice();
  init_info.QueueFamily = device_->queue_family_indicies_.graphics;
  init_info.Queue = device_->queues.graphics;
  init_info.PipelineCache = VK_NULL_HANDLE;
  init_info.DescriptorPoolSize = frame_count;
  init_info.Subpass = 0;
  init_info.MinImageCount = 2;
  init_info.ImageCount = frame_count;
  init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
  init_info.Allocator = VK_NULL_HANDLE;
  init_info.UseDynamicRendering = true;
  init_info.CheckVkResultFn = CheckVkResult;
  init_info.PipelineRenderingCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
      .colorAttachmentCount = 1,
      .pColorAttachmentFormats = &COLOR_FORMAT,
      .depthAttachmentFormat = DEPTH_FORMAT,
      //.stencilAttachmentFormat = DEPTH_FORMAT,
  };
  ImGui_ImplGlfw_InitForVulkan(window->Get(), true);
  ImGui_ImplVulkan_Init(&init_info);
  ImGui_ImplVulkan_CreateFontsTexture();
}
ImGuiStage::~ImGuiStage() {
  ImGui_ImplGlfw_Shutdown();
  ImGui_ImplVulkan_Shutdown();
  ImGui::DestroyContext();
}

void ImGuiStage::Resize() {}

void ImGuiStage::Run(uint32_t frame_index, VkSemaphore wait_semaphore,
                     uint64_t wait_value, VkPipelineStageFlags wait_flag,
                     VkSemaphore signal_semaphore, uint64_t signal_value,
                     VkFence fence) {
  auto ui_buf = Begin(frame_index);
  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
  ImGui::ShowDemoWindow();

  ImGui::Render();
  ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), ui_buf);
  End(frame_index);

  VkTimelineSemaphoreSubmitInfo timeline_info;
  VkSubmitInfo submit_info =
      PrepareSubmitInfo(timeline_info, wait_semaphore, wait_value, wait_flag,
                        signal_semaphore, signal_value);

  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &ui_buf;

  VK_CHECK_RESULT(
      vkQueueSubmit(device_->queues.compute, 1, &submit_info, fence));
}
}  // namespace blu::core::rendering