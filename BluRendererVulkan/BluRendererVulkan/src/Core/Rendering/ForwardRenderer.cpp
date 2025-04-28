#include "ForwardRenderer.h"

#include <cassert>

#include "../External/FileManager.h"
#include "Vulkan/Tools.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../External/stb_image.h"
#include "ForwardRendererConsts.h"

ForwardRenderer::ForwardRenderer(blu::core::Window* window)
    : BluCoreRenderer(window) {
  auto frame_count = swapchain_->GetImageCount();

  // Command Pool Creation
  {
    transfer_command_pools_.resize(frame_count);
    graphics_command_pools_.resize(frame_count);
    compute_command_pools_.resize(frame_count);

    VkCommandPoolCreateInfo command_pool_create{
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
    };

    for (size_t i = 0; i < frame_count; i++) {
      command_pool_create.queueFamilyIndex =
          device_->queue_family_indicies_.transfer;
      transfer_command_pools_[i] = CreateCommandPool(command_pool_create);
      command_pool_create.queueFamilyIndex =
          device_->queue_family_indicies_.graphics;
      graphics_command_pools_[i] = CreateCommandPool(command_pool_create);
      command_pool_create.queueFamilyIndex =
          device_->queue_family_indicies_.compute;
      compute_command_pools_[i] = CreateCommandPool(command_pool_create);
    }
  }

  // Descriptor Pool Create
  {
    eastl::vector<VkDescriptorPoolSize> pool_sizes{
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
         FINAL_COMPOSITION_MAX_IMAGES * frame_count},
    };

    VkDescriptorPoolCreateInfo descriptor_pool_create{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT_EXT,
        .maxSets = frame_count,
        .poolSizeCount = static_cast<uint32_t>(pool_sizes.size()),
        .pPoolSizes = pool_sizes.data(),
    };

    render_descriptor_pool_ = CreateDescriptorPool(descriptor_pool_create);
  }

  matrix_buffer_ = CreateBuffer(sizeof(glm::mat4) * (3 + MAX_MODELS),
                                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                                    VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
                                    VK_MEMORY_PROPERTY_HOST_CACHED_BIT,
                                VMA_ALLOCATION_CREATE_MAPPED_BIT);
#if DEBUG_LABELS
  VkDebugUtilsObjectNameInfoEXT debug_info{
      .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
      .objectType = VK_OBJECT_TYPE_BUFFER,
      .objectHandle = (uint64_t)matrix_buffer_->buffer,
      .pObjectName = "Matrix Buffer",
  };

  debug_util_.vkSetDebugUtilsObjectNameEXT(device_->GetLogicalDevice(),
                                           &debug_info);
#endif

  model_data_buffer_ =
      CreateBuffer(sizeof(GPUModelIndices) * MAX_MODELS,
                   VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                       VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                       VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
                       VK_MEMORY_PROPERTY_HOST_CACHED_BIT,
                   VMA_ALLOCATION_CREATE_MAPPED_BIT);

#if DEBUG_LABELS
  debug_info = {
      .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
      .objectType = VK_OBJECT_TYPE_BUFFER,
      .objectHandle = (uint64_t)model_data_buffer_->buffer,
      .pObjectName = "Model Data Buffer",
  };

  debug_util_.vkSetDebugUtilsObjectNameEXT(device_->GetLogicalDevice(),
                                           &debug_info);
#endif

  model_instance_info_buffer_ =
      CreateBuffer(sizeof(glm::vec4) * 6 + sizeof(GPUModelData) * MAX_MODELS,
                   VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                       VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                       VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
                       VK_MEMORY_PROPERTY_HOST_CACHED_BIT,
                   VMA_ALLOCATION_CREATE_MAPPED_BIT);
#if DEBUG_LABELS
  debug_info = {
      .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
      .objectType = VK_OBJECT_TYPE_BUFFER,
      .objectHandle = (uint64_t)model_instance_info_buffer_->buffer,
      .pObjectName = "Instance Model Info Buffer",
  };

  debug_util_.vkSetDebugUtilsObjectNameEXT(device_->GetLogicalDevice(),
                                           &debug_info);
#endif

  // This is horrific and should be illigal to do...
  // but it works. I WILL fix this at somepoint soon
  bda_buffer_infos_.push_back(matrix_buffer_->GetBufferInfo());
  bda_buffer_infos_.push_back(model_data_buffer_->GetBufferInfo());
  bda_buffer_infos_.push_back(model_instance_info_buffer_->GetBufferInfo());
  memcpy(bda_buffer_->mapped_data, bda_buffer_infos_.data(),
         bda_buffer_infos_.size() * sizeof(BufferInfo));

  VkCommandBufferAllocateInfo command_buffer_alloc_info{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .pNext = nullptr,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = 1};

  VkSemaphoreTypeCreateInfoKHR semaphore_type_create_info{
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO_KHR,
      .semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE_KHR,
      .initialValue = 0};
  VkSemaphoreCreateInfo binary_semaphore_info{
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
  VkSemaphoreCreateInfo timeline_semaphore_info{
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
      .pNext = &semaphore_type_create_info};

  VkFenceCreateInfo fence_info{.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
                               .flags = VK_FENCE_CREATE_SIGNALED_BIT};

  main_frame_semaphore_ = CreateSemaphore(timeline_semaphore_info);

  present_semaphores_.resize(frame_count);
  for (size_t i = 0; i < frame_count; i++) {
    present_semaphores_[i] = CreateSemaphore(binary_semaphore_info);
  }

  // UI Pass
  {
    ui_pass_.imgui_stage_ =
        new stage::ImGuiStage(instance_, device_, window, frame_count);
    RegisterStage((blu::core::rendering::Stage*)ui_pass_.imgui_stage_);
    ui_pass_.output_.resize(frame_count);
    ui_pass_.cmd_bufs_.resize(frame_count);
    ui_pass_.semaphores_.resize(frame_count);
    ui_pass_.fences_.resize(frame_count);
    for (size_t i = 0; i < frame_count; i++) {
      ui_pass_.output_[i] = CreateRenderTargetImage(
          COLOR_FORMAT, swapchain_->GetWidth(), swapchain_->GetHeight(), 1,
          VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TILING_OPTIMAL,
          VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
              VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

      VkImageSubresourceRange range{.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                                    .baseMipLevel = 0,
                                    .levelCount = VK_REMAINING_MIP_LEVELS,
                                    .baseArrayLayer = 0,
                                    .layerCount = VK_REMAINING_ARRAY_LAYERS};

      blu::core::Image::CreateImageView(device_->GetLogicalDevice(),
                                        ui_pass_.output_[i], COLOR_FORMAT,
                                        range);
      blu::core::Image::CreateImageSampler(device_->GetLogicalDevice(),
                                           ui_pass_.output_[i], 0);

      ui_pass_.cmd_bufs_[i] = AllocateCommandBuffers(graphics_command_pools_[i],
                                                     command_buffer_alloc_info);
      ui_pass_.semaphores_[i] = CreateSemaphore(binary_semaphore_info);
      ui_pass_.fences_[i] = CreateFence(fence_info);
    }
  }

  // Culling Pass
  {
    culling_pass_.build_command_buffer_stage_ =
        new stage::BuildCommandBufferStage(
            device_, LoadShader("shaders/build_command_buffer.comp.spv",
                                VK_SHADER_STAGE_COMPUTE_BIT));
    RegisterStage((blu::core::rendering::Stage*)
                      culling_pass_.build_command_buffer_stage_);
    culling_pass_.frustum_cull_stage_ = new stage::FrustumCullStage(
        device_, LoadShader("shaders/frustum_cull.comp.spv",
                            VK_SHADER_STAGE_COMPUTE_BIT));
    RegisterStage(
        (blu::core::rendering::Stage*)culling_pass_.frustum_cull_stage_);

    culling_pass_.output_draw_bufs_.resize(frame_count);
    culling_pass_.cmd_bufs_.resize(frame_count);
    culling_pass_.semaphores_.resize(frame_count);
    culling_pass_.fences_.resize(frame_count);
    for (size_t i = 0; i < frame_count; i++) {
      culling_pass_.output_draw_bufs_[i] = CreateRenderTargetBuffer(
          DRAW_COMMAND_BUFFER_SIZE * MAX_MODELS + sizeof(uint32_t),
          VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
              VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT |
              VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
              VK_BUFFER_USAGE_TRANSFER_DST_BIT,
          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

#if DEBUG_LABELS
      VkDebugUtilsObjectNameInfoEXT debug_info{
          .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
          .objectType = VK_OBJECT_TYPE_BUFFER,
          .objectHandle = (uint64_t)culling_pass_.output_draw_bufs_[i]->buffer,
          .pObjectName = "Draw Commands",
      };
      debug_util_.vkSetDebugUtilsObjectNameEXT(device_->GetLogicalDevice(),
                                               &debug_info);
#endif

      culling_pass_.cmd_bufs_[i] = AllocateCommandBuffers(
          compute_command_pools_[i], command_buffer_alloc_info);
      culling_pass_.semaphores_[i] = CreateSemaphore(binary_semaphore_info);
      culling_pass_.fences_[i] = CreateFence(fence_info);
    }
  }

  // Opaque Pass
  {
    opaque_pass_.opaque_render_stage_ = new stage::OpaqueRenderStage(
        device_,
        {
            bda_buffer_descriptor_set_layout_,
            textures_descriptor_set_layout_,
        },
        {LoadShader("shaders/opaque_pass.vert.spv", VK_SHADER_STAGE_VERTEX_BIT),
         LoadShader("shaders/opaque_pass.frag.spv",
                    VK_SHADER_STAGE_FRAGMENT_BIT)});
    RegisterStage(
        (blu::core::rendering::Stage*)opaque_pass_.opaque_render_stage_);

    VkImageSubresourceRange range{
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = VK_REMAINING_ARRAY_LAYERS,
    };

    VkImageSubresourceRange depth_range{
        .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = VK_REMAINING_ARRAY_LAYERS,
    };

    opaque_pass_.color_output_.resize(frame_count);
    opaque_pass_.depth_output_.resize(frame_count);
    opaque_pass_.cmd_bufs_.resize(frame_count);
    opaque_pass_.semaphores_.resize(frame_count);
    opaque_pass_.fences_.resize(frame_count);
    for (size_t i = 0; i < frame_count; i++) {
      opaque_pass_.color_output_[i] = CreateRenderTargetImage(
          COLOR_FORMAT, swapchain_->GetWidth(), swapchain_->GetHeight(), 1,
          VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TILING_OPTIMAL,
          VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
              VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
      blu::core::Image::CreateImageView(device_->GetLogicalDevice(),
                                        opaque_pass_.color_output_[i],
                                        COLOR_FORMAT, range);

      blu::core::Image::CreateImageSampler(device_->GetLogicalDevice(),
                                           opaque_pass_.color_output_[i], 0);

      opaque_pass_.depth_output_[i] = CreateRenderTargetImage(
          DEPTH_FORMAT, swapchain_->GetWidth(), swapchain_->GetHeight(), 1,
          VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TILING_OPTIMAL,
          VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

      blu::core::Image::CreateImageView(device_->GetLogicalDevice(),
                                        opaque_pass_.depth_output_[i],
                                        DEPTH_FORMAT, depth_range);

#if DEBUG_LABELS
      VkDebugUtilsObjectNameInfoEXT debug_info{
          .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
          .objectType = VK_OBJECT_TYPE_IMAGE,
          .objectHandle = (uint64_t)opaque_pass_.color_output_[i]->image,
          .pObjectName = "Opaque Pass Color Output",
      };
      debug_util_.vkSetDebugUtilsObjectNameEXT(device_->GetLogicalDevice(),
                                               &debug_info);
      debug_info.objectHandle = (uint64_t)opaque_pass_.depth_output_[i]->image;
      debug_info.pObjectName = "Opaque Pass Depth Output";
      debug_util_.vkSetDebugUtilsObjectNameEXT(device_->GetLogicalDevice(),
                                               &debug_info);
#endif

      opaque_pass_.cmd_bufs_[i] = AllocateCommandBuffers(
          graphics_command_pools_[i], command_buffer_alloc_info);
      opaque_pass_.semaphores_[i] = CreateSemaphore(binary_semaphore_info);
      opaque_pass_.fences_[i] = CreateFence(fence_info);
    }
  }

  // Post Processing Pass
  {
    VkDescriptorSetLayoutBinding binding = {
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = FINAL_COMPOSITION_MAX_IMAGES,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        .pImmutableSamplers = nullptr,
    };

    VkDescriptorSetLayoutCreateInfo layout_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = 1,
        .pBindings = &binding,
    };
    post_processing_pass_.ui_composition_descriptor_set_layout_ =
        CreateDescriptorSetLayout(layout_info);

    post_processing_pass_.ui_composition_descriptor_sets_.resize(frame_count);
    for (size_t i = 0; i < frame_count; i++) {
      VkDescriptorSetAllocateInfo alloc_info = {
          .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
          .pNext = nullptr,
          .descriptorPool = render_descriptor_pool_,
          .descriptorSetCount = 1,
          .pSetLayouts =
              &post_processing_pass_.ui_composition_descriptor_set_layout_,
      };

      VK_CHECK_RESULT(vkAllocateDescriptorSets(
          device_->GetLogicalDevice(), &alloc_info,
          &post_processing_pass_.ui_composition_descriptor_sets_[i]));

      eastl::vector<VkDescriptorImageInfo> imageInfos;
      imageInfos.resize(FINAL_COMPOSITION_MAX_IMAGES);
      imageInfos[0] = {
          .sampler = opaque_pass_.color_output_[i]->sampler,
          .imageView = opaque_pass_.color_output_[i]->view,
          .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      };
      imageInfos[1] = {
          .sampler = ui_pass_.output_[i]->sampler,
          .imageView = ui_pass_.output_[i]->view,
          .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      };

      VkWriteDescriptorSet write = {
          .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
          .dstSet = post_processing_pass_.ui_composition_descriptor_sets_[i],
          .dstBinding = 0,
          .descriptorCount = static_cast<uint32_t>(imageInfos.size()),
          .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
          .pImageInfo = imageInfos.data(),
      };

      vkUpdateDescriptorSets(device_->GetLogicalDevice(), 1, &write, 0,
                             nullptr);
    }

    post_processing_pass_.ui_composition_stage_ = new stage::ColorOnlyStage(
        device_, {post_processing_pass_.ui_composition_descriptor_set_layout_},
        {LoadShader("shaders/fullscreen_tri.vert.spv",
                    VK_SHADER_STAGE_VERTEX_BIT),
         LoadShader("shaders/final_composition.frag.spv",
                    VK_SHADER_STAGE_FRAGMENT_BIT)});
    RegisterStage((blu::core::rendering::Stage*)
                      post_processing_pass_.ui_composition_stage_);

    VkImageSubresourceRange range{
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = VK_REMAINING_ARRAY_LAYERS,
    };

    post_processing_pass_.cmd_bufs_.resize(frame_count);
    post_processing_pass_.semaphores_.resize(frame_count);
    post_processing_pass_.fences_.resize(frame_count);
    for (size_t i = 0; i < frame_count; i++) {
      post_processing_pass_.cmd_bufs_[i] = AllocateCommandBuffers(
          graphics_command_pools_[i], command_buffer_alloc_info);
      post_processing_pass_.semaphores_[i] =
          CreateSemaphore(binary_semaphore_info);
      post_processing_pass_.fences_[i] = CreateFence(fence_info);
    }
  }
}

void ForwardRenderer::BuildFrameTimeline() {
  semaphore_values_ = {};

  switch (settings_.culling_mode) {
    case ForwardRenderSettings::CULLING_MODE_NONE:
      semaphore_values_.build_command_buffer_stage_ =
          GetNextSemaphoreValue();  // 1 buf
      break;
    case ForwardRenderSettings::CULLING_MODE_FRUSTUM_CULL:
      semaphore_values_.frustum_cull_stage_ = GetNextSemaphoreValue();  // 1 buf
      break;
    case ForwardRenderSettings::CULLING_MODE_OCCLUSION_CULL:
      semaphore_values_.frustum_cull_stage_ = GetNextSemaphoreValue();  // 1 buf
      semaphore_values_.depth_only_stage_ =
          GetNextSemaphoreValue();  // 1 depth img
      semaphore_values_.occlusion_cull_stage_ =
          GetNextSemaphoreValue();  // 1 buf
      break;
  }
  semaphore_values_.cull_mode_complete = current_semaphore_value_;

  switch (settings_.draw_mode) {
    case ForwardRenderSettings::DRAW_MODE_SHADED:
      semaphore_values_.opaque_render_stage_ =
          GetNextSemaphoreValue();  // 1 depth img 1 color img
      break;
    case ForwardRenderSettings::DRAW_MODE_UNLIT:
      semaphore_values_.unlit_opaque_render_stage_ =
          GetNextSemaphoreValue();  // 1 depth img 1 color img
      break;
    case ForwardRenderSettings::DRAW_MODE_WIREFRAME:
      semaphore_values_.wireframe_render_stage_ =
          GetNextSemaphoreValue();  // 1 depth img 1 color img
      break;
  }
  semaphore_values_.draw_mode_complete = current_semaphore_value_;
  if (settings_.output == ForwardRenderSettings::RENDER_OUTPUT_DRAW_STAGE)
    return;

  switch (settings_.aliasing) {
    case ForwardRenderSettings::ANTI_ALIAS_MODE_NONE:
      break;
    case ForwardRenderSettings::ANTI_ALIAS_MODE_FXAA:
      semaphore_values_.anti_aliasing_stage_ =
          GetNextSemaphoreValue();  // 1 (storage) color img
      break;
  }
  semaphore_values_.anti_aliasing_mode_complete = current_semaphore_value_;
  if (settings_.output == ForwardRenderSettings::RENDER_OUTPUT_AA) return;
}

void ForwardRenderer::UpdateFrameData(RenderData& render_data) {
  memcpy(model_data_buffer_->mapped_data, model_indices_.data(),
         model_indices_.size() * sizeof(GPUModelIndices));

  memcpy(model_instance_info_buffer_->mapped_data, render_data.planes,
         sizeof(glm::vec4) * 6);
  memcpy(model_instance_info_buffer_->mapped_data + sizeof(glm::vec4) * 6,
         render_data.model_data.data(),
         sizeof(GPUModelData) * render_data.model_data.size());

  memcpy(matrix_buffer_->mapped_data, render_data.matrices.data(),
         render_data.matrices.size() * sizeof(glm::mat4));
}

void ForwardRenderer::Render(RenderData render_data) {
  BuildFrameTimeline();
  UpdateFrameData(render_data);

  UiPass();
  CullingPass(render_data.model_data.size());
  OpaquePass();
  PostProcessingPass();

  if (!DoPresent()) {
    Resize();
  }

  frame_index_ = (frame_index_ + 1) % swapchain_->GetImageCount();
}

void ForwardRenderer::Resize() {
  vkDeviceWaitIdle(device_->GetLogicalDevice());

  swapchain_->Create(false, false);

  auto frame_count = swapchain_->GetImageCount();
  auto width = swapchain_->GetWidth();
  auto height = swapchain_->GetHeight();
}

void ForwardRenderer::BuildImGui() {
  // Render Settings (Required)
  {
    ImGui::Begin("Render Settings");
    ImGui::SetWindowPos(ImVec2(0, swapchain_->GetHeight() / 4), ImGuiCond_Once);
    ImGui::SetWindowSize(ImVec2(250, 300), ImGuiCond_Once);
    if (ImGui::BeginCombo("UI Mode", settings_.uiModes[settings_.ui_mode], ImGuiComboFlags_WidthFitPreview)) {
      uint32_t curr = settings_.ui_mode;
      for (int n = 0; n < settings_.uiModes.size(); n++) {
        bool is_selected = (curr == n);
        if (ImGui::Selectable(settings_.uiModes[n], is_selected)) {
          settings_.ui_mode = (ForwardRenderSettings::UiMode)n;
        }
        if (is_selected) ImGui::SetItemDefaultFocus();
      }
      ImGui::EndCombo();
    }
    if (ImGui::BeginCombo("Culling Mode",
                          settings_.cullingModes[settings_.culling_mode],
                          ImGuiComboFlags_WidthFitPreview)) {
      uint32_t curr = settings_.culling_mode;
      for (int n = 0; n < settings_.cullingModes.size(); n++) {
        bool is_selected = (curr == n);
        if (ImGui::Selectable(settings_.cullingModes[n], is_selected)) {
          settings_.culling_mode = (ForwardRenderSettings::CullingMode)n;
        }
        if (is_selected) ImGui::SetItemDefaultFocus();
      }
      ImGui::EndCombo();
    }
    if (ImGui::BeginCombo("Output", settings_.renderOutputs[settings_.output],
                          ImGuiComboFlags_WidthFitPreview)) {
      uint32_t curr = settings_.output;
      for (int n = 0; n < settings_.renderOutputs.size(); n++) {
        bool is_selected = (curr == n);
        if (ImGui::Selectable(settings_.renderOutputs[n], is_selected)) {
          settings_.output = (ForwardRenderSettings::RenderOutput)n;
        }
        if (is_selected) ImGui::SetItemDefaultFocus();
      }
      ImGui::EndCombo();
    }
    ImGui::End();
  }

  if (settings_.ui_mode == ForwardRenderSettings::UI_MODE_MINIMAL) return;

  // Debug Data (Optional)
  {
    ImGui::Begin("Debug", 0,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);
    ImGui::SetWindowPos(ImVec2(0, 0), ImGuiCond_Once);
    ImGui::SetWindowSize(ImVec2(200, 50), ImGuiCond_Once);

    ImGui::TextUnformatted(device_->GetDeviceProperties().deviceName);
    ImGui::Text(
        "Vulkan API %i.%i.%i",
        VK_API_VERSION_MAJOR(device_->GetDeviceProperties().apiVersion),
        VK_API_VERSION_MINOR(device_->GetDeviceProperties().apiVersion),
        VK_API_VERSION_PATCH(device_->GetDeviceProperties().apiVersion));

    ImGui::End();
  }

  BuildCoreDebugUI();
}

void ForwardRenderer::UiPass() {
  vkWaitForFences(device_->GetLogicalDevice(), 1,
                  &ui_pass_.fences_[frame_index_], VK_TRUE, UINT64_MAX);
  vkResetFences(device_->GetLogicalDevice(), 1,
                &ui_pass_.fences_[frame_index_]);

  auto buf = ui_pass_.cmd_bufs_[frame_index_];
  StartCommandBuffer(buf, "UI PASS", {0, 1, 0, 1});

  ui_pass_.imgui_stage_->Start(buf, ui_pass_.output_[frame_index_],
                               swapchain_->GetWidth(), swapchain_->GetHeight());

  BuildImGui();

  ui_pass_.imgui_stage_->End(buf);
  EndCommandBuffer(buf);
  SubmitCommandBuffer({buf}, device_->queues.graphics, {}, {}, {},
                      {ui_pass_.semaphores_[frame_index_]}, {0},
                      ui_pass_.fences_[frame_index_]);
}

void ForwardRenderer::CullingPass(uint32_t model_count) {
  vkWaitForFences(device_->GetLogicalDevice(), 1,
                  &culling_pass_.fences_[frame_index_], VK_TRUE, UINT64_MAX);
  vkResetFences(device_->GetLogicalDevice(), 1,
                &culling_pass_.fences_[frame_index_]);

  VkCommandBuffer buf = culling_pass_.cmd_bufs_[frame_index_];
  StartCommandBuffer(buf, "CULLING PASS", {1, 0, 0, 1});

  switch (settings_.culling_mode) {
    case ForwardRenderSettings::CULLING_MODE_NONE:
      culling_pass_.build_command_buffer_stage_->Run(
          buf, model_data_buffer_->GetBufferInfo(),
          model_instance_info_buffer_->GetBufferInfo(), model_count,
          culling_pass_.output_draw_bufs_[frame_index_]);
      break;
    case ForwardRenderSettings::CULLING_MODE_FRUSTUM_CULL:
      culling_pass_.frustum_cull_stage_->Run(
          buf, model_data_buffer_->GetBufferInfo(),
          model_instance_info_buffer_->GetBufferInfo(), model_count,
          culling_pass_.output_draw_bufs_[frame_index_]);
      break;
    case ForwardRenderSettings::CULLING_MODE_OCCLUSION_CULL:
      break;
  }

  EndCommandBuffer(buf);
  SubmitCommandBuffer({buf}, device_->queues.compute, {}, {}, {},
                      {main_frame_semaphore_},
                      {semaphore_values_.cull_mode_complete},
                      culling_pass_.fences_[frame_index_]);
}

void ForwardRenderer::OpaquePass() {
  vkWaitForFences(device_->GetLogicalDevice(), 1,
                  &opaque_pass_.fences_[frame_index_], VK_TRUE, UINT64_MAX);
  vkResetFences(device_->GetLogicalDevice(), 1,
                &opaque_pass_.fences_[frame_index_]);

  VkCommandBuffer buf = opaque_pass_.cmd_bufs_[frame_index_];
  StartCommandBuffer(buf, "OPAQUE PASS", {0, 1, 0, 1});

  switch (settings_.draw_mode) {
    case ForwardRenderSettings::DRAW_MODE_SHADED:
      opaque_pass_.opaque_render_stage_->Run(
          buf, opaque_pass_.color_output_[frame_index_],
          opaque_pass_.depth_output_[frame_index_], swapchain_->GetWidth(),
          swapchain_->GetHeight(),
          culling_pass_.output_draw_bufs_[frame_index_],
          {bda_buffer_descriptor_set_, textures_descriptor_set_},
          vertex_buffer_, normal_buffer_, uv_buffer_, index_buffer_);
      break;
  }

  EndCommandBuffer(buf);
  SubmitCommandBuffer({buf}, device_->queues.graphics, {main_frame_semaphore_},
                      {semaphore_values_.cull_mode_complete},
                      {VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT},
                      {main_frame_semaphore_},
                      {semaphore_values_.draw_mode_complete},
                      opaque_pass_.fences_[frame_index_]);
}

void ForwardRenderer::PostProcessingPass() {
  vkWaitForFences(device_->GetLogicalDevice(), 1,
                  &post_processing_pass_.fences_[frame_index_], VK_TRUE,
                  UINT64_MAX);

  auto acquire_image_result = vkAcquireNextImageKHR(
      device_->GetLogicalDevice(), swapchain_->GetSwapchain(), UINT64_MAX,
      post_processing_pass_.semaphores_[frame_index_], nullptr, &image_index_);

  if ((acquire_image_result == VK_ERROR_OUT_OF_DATE_KHR) ||
      (acquire_image_result == VK_SUBOPTIMAL_KHR)) {
    assert(false);
  }

  vkResetFences(device_->GetLogicalDevice(), 1,
                &post_processing_pass_.fences_[frame_index_]);

  auto swapchain_buf = swapchain_->GetSwapchainBuffer(frame_index_);

  VkCommandBuffer buf = post_processing_pass_.cmd_bufs_[frame_index_];
  StartCommandBuffer(buf, "POST PROCESSING PASS", {0, 1, 0, 1});

  VkImageSubresourceRange range{
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .baseMipLevel = 0,
      .levelCount = 1,
      .baseArrayLayer = 0,
      .layerCount = 1,
  };
  blu::core::Image::ImageLayoutTransition(
      buf, ui_pass_.output_[frame_index_]->image,
      VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, range);
  blu::core::Image::ImageLayoutTransition(
      buf, opaque_pass_.color_output_[frame_index_]->image,
      VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, range);

  blu::core::Image img{.image = swapchain_buf.image,
                       .view = swapchain_buf.view};

  VkDescriptorImageInfo image_info = {
      .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
  };

  switch (settings_.output) {
    case ForwardRenderSettings::RENDER_OUTPUT_DRAW_STAGE:
      image_info.sampler = opaque_pass_.color_output_[frame_index_]->sampler;
      image_info.imageView = opaque_pass_.color_output_[frame_index_]->view;
      break;
    case ForwardRenderSettings::RENDER_OUTPUT_AA:
      image_info.sampler = opaque_pass_.color_output_[frame_index_]->sampler;
      image_info.imageView = opaque_pass_.color_output_[frame_index_]->view;
      break;
    case ForwardRenderSettings::RENDER_OUTPUT_FINAL:
      image_info.sampler = opaque_pass_.color_output_[frame_index_]->sampler;
      image_info.imageView = opaque_pass_.color_output_[frame_index_]->view;
      break;
  }

  VkWriteDescriptorSet write_descriptor_set = {
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .dstSet =
          post_processing_pass_.ui_composition_descriptor_sets_[frame_index_],
      .dstBinding = 0,
      .dstArrayElement = 0,
      .descriptorCount = 1,
      .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
      .pImageInfo = &image_info,
  };

  vkUpdateDescriptorSets(device_->GetLogicalDevice(), 1, &write_descriptor_set,
                         0, nullptr);

  post_processing_pass_.ui_composition_stage_->Run(
      buf,
      {post_processing_pass_.ui_composition_descriptor_sets_[frame_index_]},
      &img, swapchain_->GetWidth(), swapchain_->GetHeight());

  blu::core::Image::ImageLayoutTransition(
      buf, swapchain_buf.image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, range);
  EndCommandBuffer(buf);

  SubmitCommandBuffer(
      {buf}, device_->queues.graphics,
      {main_frame_semaphore_, ui_pass_.semaphores_[frame_index_]},
      {semaphore_values_.draw_mode_complete, 0},
      {VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
       VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT},
      {present_semaphores_[frame_index_]}, {0},
      post_processing_pass_.fences_[frame_index_]);
}

bool ForwardRenderer::DoPresent() {
  VkSemaphore present_wait_semaphore[] = {
      present_semaphores_[frame_index_],
      post_processing_pass_.semaphores_[frame_index_]};

  auto swapchain = swapchain_->GetSwapchain();
  VkPresentInfoKHR present_info = {
      .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
      .pNext = NULL,
      .waitSemaphoreCount = 2,
      .pWaitSemaphores = present_wait_semaphore,
      .swapchainCount = 1,
      .pSwapchains = &swapchain,
      .pImageIndices = &image_index_,
  };

  auto queue_present_result =
      vkQueuePresentKHR(device_->queues.graphics, &present_info);
  if ((queue_present_result == VK_ERROR_OUT_OF_DATE_KHR) ||
      (queue_present_result == VK_SUBOPTIMAL_KHR)) {
    return false;
  }
  return true;
}