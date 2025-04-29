#ifndef FORWARDRENDERER_H
#define FORWARDRENDERER_H

#include <EASTL/array.h>

#include "BluCoreRenderer.h"
#include "RenderData.h"
#include "Stages/AntiAliasingStage.h"
#include "Stages/BuildCommandBufferStage.h"
#include "Stages/ColorOnlyStage.h"
#include "Stages/DepthOnlyStage.h"
#include "Stages/FrustumCullStage.h"
#include "Stages/ImGuiStage.h"
#include "Stages/ImageCopyStage.h"
#include "Stages/OpaqueRenderStage.h"

namespace stage = blu::core::rendering::stage;

class ForwardRenderer : public BluCoreRenderer {
 public:
  ForwardRenderer(blu::core::Window* window);

  void Render(RenderData render_data);

 protected:
  void Resize() override;

 private:
  void BuildFrameTimeline();
  void UpdateFrameData(RenderData&);

  void BuildImGui();
  void UiPass();
  void CullingPass(uint32_t model_count);
  void OpaquePass();
  void DoAA();
  void PostProcessingPass();

  bool DoPresent();

  struct TimelineSemaphoreValues {
    // Complete Operations
    uint64_t draw_mode_ready = 0;
    uint64_t anti_aliasing_ready = 0;
    uint64_t present_ready = 0;

    // Individual Stages
    uint64_t build_command_buffer_stage_ = 0;
    uint64_t frustum_cull_stage_ = 0;
    uint64_t depth_only_stage_ = 0;
    uint64_t occlusion_cull_stage_ = 0;
    uint64_t opaque_render_stage_ = 0;
    uint64_t unlit_opaque_render_stage_ = 0;
    uint64_t wireframe_render_stage_ = 0;
    uint64_t image_copy_stage_ = 0;
    uint64_t anti_aliasing_stage_ = 0;
  } semaphore_values_;

  struct ForwardRenderSettings {
    eastl::array<const char*, 2> uiModes{"Minimal", "Full"};
    enum UiMode {
      UI_MODE_MINIMAL = 0,
      UI_MODE_FULL = 1,
    } ui_mode = UI_MODE_FULL;
    eastl::array<const char*, 3> cullingModes{"None", "Frustum Cull",
                                              "Occlusion Cull"};
    enum CullingMode {
      CULLING_MODE_NONE = 0,
      CULLING_MODE_FRUSTUM_CULL = 1,
      CULLING_MODE_OCCLUSION_CULL = 2,
    } culling_mode = CULLING_MODE_FRUSTUM_CULL;
    eastl::array<const char*, 3> drawModes{"Shaded", "Unlit", "Wireframe"};
    enum DrawMode {
      DRAW_MODE_SHADED = 0,
      DRAW_MODE_UNLIT = 1,
      DRAW_MODE_WIREFRAME = 2,
    } draw_mode = DRAW_MODE_SHADED;
    eastl::array<const char*, 2> antiAliasingModes{"None", "FXAA"};
    enum AntiAliasingMode {
      ANTI_ALIAS_MODE_NONE = 0,
      ANTI_ALIAS_MODE_FXAA = 1,
    } aa_mode = ANTI_ALIAS_MODE_NONE;
    eastl::array<const char*, 3> renderOutputs{"Draw", "AA", "FINAL"};
    enum RenderOutput {
      RENDER_OUTPUT_DRAW_STAGE = 0,
      RENDER_OUTPUT_AA = 1,
      RENDER_OUTPUT_FINAL = 2,
    } output = RenderOutput::RENDER_OUTPUT_DRAW_STAGE;
  } settings_;

  // Vulkan Render Data
  // Descriptor Resources
  eastl::vector<CommandPool> graphics_command_pools_;
  eastl::vector<CommandPool> compute_command_pools_;
  eastl::vector<CommandPool> transfer_command_pools_;

  VkDescriptorPool render_descriptor_pool_;

  blu::core::Buffer* matrix_buffer_;

  blu::core::Buffer* model_data_buffer_;
  blu::core::Buffer* model_instance_info_buffer_;

  VkSemaphore main_frame_semaphore_;
  eastl::vector<VkSemaphore> present_semaphores_;

  struct UiPass : Pass {
    stage::ImGuiStage* imgui_stage_ = nullptr;

    eastl::vector<blu::core::Image*> output_;
  } ui_pass_;

  struct CullingPass : Pass {
    stage::BuildCommandBufferStage* build_command_buffer_stage_ = nullptr;
    stage::FrustumCullStage* frustum_cull_stage_ = nullptr;
    stage::DepthOnlyStage* depth_only_stage_ = nullptr;
    // TODO
    blu::core::rendering::Stage* hi_z_stage_;
    blu::core::rendering::Stage* occlusion_cull_stage_;

    eastl::vector<blu::core::Buffer*> output_draw_bufs_;
  } culling_pass_;

  struct OpaquePass : Pass {
    stage::OpaqueRenderStage* opaque_render_stage_ = nullptr;

    eastl::vector<blu::core::Image*> color_output_;
    eastl::vector<blu::core::Image*> depth_output_;
  } opaque_pass_;

  struct AntiAliasingPass : Pass {
    stage::AntiAliasingStage* anti_aliasing_stage_ = nullptr;

    VkDescriptorSetLayout aa_descriptor_set_layout_;
    eastl::vector<VkDescriptorSet> aa_descriptor_sets_;
    eastl::vector<blu::core::Image*> output_;
  } aa_pass_;

  struct PostProcessingPass : Pass {
    stage::ColorOnlyStage* ui_composition_stage_ = nullptr;
    // stage::ColorOnlyStage* image_blit_stage_ = nullptr;

    VkDescriptorSetLayout ui_composition_descriptor_set_layout_;
    eastl::vector<VkDescriptorSet> ui_composition_descriptor_sets_;
  } post_processing_pass_;
};

#endif