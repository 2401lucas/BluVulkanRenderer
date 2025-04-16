#include "ImGuiStage.h"

#include <EASTL/array.h>

static void CheckVkResult(VkResult err) {
  if (err == 0) return;
  fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
  if (err < 0) abort();
}

namespace blu::core::rendering {
ImGuiStage::ImGuiStage(Instance* instance, Device* device, Window* window,
                       uint32_t frame_count)
    : Stage(device, nullptr) {
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

void ImGuiStage::Start(VkCommandBuffer buf, Image* dst, uint32_t width,
                       uint32_t height) {
  VkImageSubresourceRange range{
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .baseMipLevel = 0,
      .levelCount = 1,
      .baseArrayLayer = 0,
      .layerCount = 1,
  };

  blu::core::Image::ImageLayoutTransition(
      buf, dst->image, VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, range);

  eastl::array<VkClearValue, 1> clear_values{};
  clear_values[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};

  VkRenderingAttachmentInfo color_attachment_info{
      .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
      .imageView = dst->view,
      .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      .resolveMode = VK_RESOLVE_MODE_NONE,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
      .clearValue = clear_values[0],
  };

  VkRenderingInfo render_info{
      .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
      .renderArea = {{.x = 0, .y = 0}, width, height},
      .layerCount = 1,
      .viewMask = 0,
      .colorAttachmentCount = 1,
      .pColorAttachments = &color_attachment_info,
  };

  vkCmdBeginRendering(buf, &render_info);

  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
  ImGui::ShowDemoWindow();
}

void ImGuiStage::End(VkCommandBuffer buf) {
  ImGui::Render();
  ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), buf);
  vkCmdEndRendering(buf);
}
}  // namespace blu::core::rendering