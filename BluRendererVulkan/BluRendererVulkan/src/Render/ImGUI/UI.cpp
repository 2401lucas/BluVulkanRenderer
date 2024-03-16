#include "UI.h"

#include "../../../include/RenderConst.h"
#include "../Descriptors/DescriptorUtils.h"

UI::UI(Device* device, CommandPool* commandPool, RenderPass* renderPass) {
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.FontGlobalScale = 1.0f;
  ImGuiStyle& style = ImGui::GetStyle();
  style.ScaleAllSizes(1.0f);

  // Color scheme
  vulkanStyle = ImGui::GetStyle();
  vulkanStyle.Colors[ImGuiCol_TitleBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.6f);
  vulkanStyle.Colors[ImGuiCol_TitleBgActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);
  vulkanStyle.Colors[ImGuiCol_MenuBarBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
  vulkanStyle.Colors[ImGuiCol_Header] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
  vulkanStyle.Colors[ImGuiCol_CheckMark] = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
  style = vulkanStyle;

  // Dimensions
  io.DisplaySize = ImVec2(1920, 1080);
  io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);

  // Create font texture
  unsigned char* fontData;
  int texWidth, texHeight;
  io.Fonts->GetTexDataAsRGBA32(&fontData, &texWidth, &texHeight);
  VkDeviceSize uploadSize = texWidth * texHeight * 4 * sizeof(char);

  // Create target image for copy
  uiImage =
      new Image(device, texWidth, texHeight, 1, VK_SAMPLE_COUNT_1_BIT,
                VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  uiImage->createImageView(device, VK_IMAGE_ASPECT_COLOR_BIT);

  // Staging buffers for font data upload
  Buffer* imgStagingBuffer =
      new Buffer(device, uploadSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  imgStagingBuffer->copyData(device, fontData, 0, uploadSize, 0);
  uiImage->transitionImageLayout(
      device, commandPool, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, std::nullopt);
  uiImage->copyImageFromBuffer(device, commandPool, imgStagingBuffer,
                               std::nullopt);

  uiImage->transitionImageLayout(device, commandPool,
                                 VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                 std::nullopt);

  imgStagingBuffer->freeBuffer(device);
  delete imgStagingBuffer;

  uiImage->createTextureSampler(device);

  // Descriptor pool
  std::vector<VkDescriptorPoolSize> poolSizes = {1, VkDescriptorPoolSize()};
  poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  poolSizes[0].descriptorCount =
      static_cast<uint32_t>(RenderConst::MAX_FRAMES_IN_FLIGHT);
  uiDescriptorPool = new DescriptorPool(device, poolSizes,
                                        RenderConst::MAX_FRAMES_IN_FLIGHT, 0);

  VkDescriptorSetLayoutBinding uiLayoutBinding =
      DescriptorUtils::createDescriptorSetBinding(
          0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, nullptr,
          VK_SHADER_STAGE_FRAGMENT_BIT);
  std::vector<VkDescriptorSetLayoutBinding> uiBindings = {uiLayoutBinding};
  uiDescriptorSetLayout = new Descriptor(device, uiBindings);

  DescriptorUtils::allocateDesriptorSets(device,
                                         uiDescriptorSetLayout->getLayout(),
                                         uiDescriptorPool, uiDescriptorSets);
  std::vector<std::vector<VkDescriptorImageInfo>> uiImageDescriptorInfos;
  for (size_t i = 0; i < RenderConst::MAX_FRAMES_IN_FLIGHT; i++) {
    std::vector<VkDescriptorImageInfo> uiImageDescriptorInfo;
    for (size_t i = 0; i < 1; i++) {
      VkDescriptorImageInfo uiImageInfo{};
      uiImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      uiImageInfo.imageView = uiImage->getImageView();
      uiImageInfo.sampler = uiImage->getImageSampler();
      uiImageDescriptorInfo.push_back(uiImageInfo);
    }
    uiImageDescriptorInfos.push_back(uiImageDescriptorInfo);
  }

  DescriptorUtils::createImageDescriptorSet(device, uiDescriptorSets,
                                            uiImageDescriptorInfos);
  std::vector<VkDescriptorSetLayout> graphicsDescriptorSetLayouts = {
      uiDescriptorSetLayout->getLayout()};

  uiPipeline = new GraphicsPipeline(
      device,
      std::pair<ShaderInfo, ShaderInfo>(ShaderInfo(VERTEX, "uiVert.spv"),
                                        ShaderInfo(FRAGMENT, "uiFrag.spv")),
      graphicsDescriptorSetLayouts, renderPass);
}

void UI::cleanup(Device* device) {
  uiDescriptorPool->cleanup(device);
  delete uiDescriptorPool;
  uiDescriptorSetLayout->cleanup(device);
  delete uiDescriptorSetLayout;
  uiPipeline->cleanup(device);
  delete uiPipeline;
  vertexBuffer->freeBuffer(device);
  delete vertexBuffer;
  indexBuffer->freeBuffer(device);
  delete indexBuffer;
  uiImage->cleanup(device);
  delete uiImage;
}

void UI::setStyle(uint32_t index) {
  switch (index) {
    case 0: {
      ImGuiStyle& style = ImGui::GetStyle();
      style = vulkanStyle;
      break;
    }
    case 1:
      ImGui::StyleColorsClassic();
      break;
    case 2:
      ImGui::StyleColorsDark();
      break;
    case 3:
      ImGui::StyleColorsLight();
      break;
  }
}

void UI::newFrame(Device* device, bool updateFrameGraph) {
  ImGui::NewFrame();

  // Init imGui windows and elements

  // Debug window
  ImGui::SetWindowPos(ImVec2(20 * 1, 20 * 1), ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowSize(ImVec2(200, 200), ImGuiCond_FirstUseEver);
  ImGui::TextUnformatted("Title");
  ImGui::TextUnformatted(device->getGPUProperties().deviceName);

  // SRS - Display Vulkan API version and device driver information if available
  // (otherwise blank)
  ImGui::Text("Vulkan API %i.%i.%i",
              VK_API_VERSION_MAJOR(device->getGPUProperties().apiVersion),
              VK_API_VERSION_MINOR(device->getGPUProperties().apiVersion),
              VK_API_VERSION_PATCH(device->getGPUProperties().apiVersion));

  // Update frame time display
  if (false) {
    std::rotate(uiSettings.frameTimes.begin(),
                uiSettings.frameTimes.begin() + 1, uiSettings.frameTimes.end());
    float frameTime = 1000.0f / (1 * 1000.0f);
    uiSettings.frameTimes.back() = frameTime;
    if (frameTime < uiSettings.frameTimeMin) {
      uiSettings.frameTimeMin = frameTime;
    }
    if (frameTime > uiSettings.frameTimeMax) {
      uiSettings.frameTimeMax = frameTime;
    }
  }

  ImGui::PlotLines("Frame Times", &uiSettings.frameTimes[0], 50, 0, "",
                   uiSettings.frameTimeMin, uiSettings.frameTimeMax,
                   ImVec2(0, 80));

  // ImGui::Text("Camera");
  // ImGui::InputFloat3("position", &example->camera.position.x, 2);
  // ImGui::InputFloat3("rotation", &example->camera.rotation.x, 2);

  // Example settings window
  ImGui::SetNextWindowPos(ImVec2(20 * 1, 360 * 1), ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowSize(ImVec2(300 * 1, 200 * 1), ImGuiCond_FirstUseEver);
  ImGui::Begin("Example settings");
  ImGui::Checkbox("Render models", &uiSettings.displayModels);
  ImGui::Checkbox("Display logos", &uiSettings.displayLogos);
  ImGui::Checkbox("Display background", &uiSettings.displayBackground);
  ImGui::Checkbox("Animate light", &uiSettings.animateLight);
  ImGui::SliderFloat("Light speed", &uiSettings.lightSpeed, 0.1f, 1.0f);
  // ImGui::ShowStyleSelector("UI style");

  if (ImGui::Combo("UI style", &selectedStyle,
                   "Vulkan\0Classic\0Dark\0Light\0")) {
    setStyle(selectedStyle);
  }

  ImGui::End();

  // SRS - ShowDemoWindow() sets its own initial position and size, cannot
  // override here
  ImGui::ShowDemoWindow();

  // Render to generate draw buffers
  ImGui::Render();
}

void UI::updateBuffers(Device* device) {
  ImDrawData* imDrawData = ImGui::GetDrawData();

  VkDeviceSize vertexBufferSize =
      imDrawData->TotalVtxCount * sizeof(ImDrawVert);
  VkDeviceSize indexBufferSize = imDrawData->TotalIdxCount * sizeof(ImDrawIdx);

  if ((vertexBufferSize == 0) || (indexBufferSize == 0)) {
    return;
  }

  if ((vertexBuffer == nullptr) || (vertexCount != imDrawData->TotalVtxCount)) {
    if (vertexBuffer != nullptr) {
      vertexBuffer->freeBuffer(device);
      delete vertexBuffer;
    }

    vertexBuffer =
        new Buffer(device, vertexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    vertexCount = imDrawData->TotalVtxCount;
  }

  // Index buffer
  if ((indexBuffer == nullptr) || (indexCount < imDrawData->TotalIdxCount)) {
    if (indexBuffer != nullptr) {
      indexBuffer->freeBuffer(device);
      delete indexBuffer;
    }

    indexBuffer =
        new Buffer(device, indexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    indexCount = imDrawData->TotalIdxCount;
  }

  // Upload data
  int offsetVtx = 0;
  int offsetIdx = 0;

  for (int n = 0; n < imDrawData->CmdListsCount; n++) {
    const ImDrawList* cmd_list = imDrawData->CmdLists[n];
    vertexBuffer->copyData(device, cmd_list->VtxBuffer.Data, offsetVtx,
                           cmd_list->VtxBuffer.Size * sizeof(ImDrawVert), 0);

    indexBuffer->copyData(device, cmd_list->IdxBuffer.Data, offsetIdx,
                          cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx), 0);
    offsetVtx += cmd_list->VtxBuffer.Size;
    offsetIdx += cmd_list->IdxBuffer.Size;
  }

  // vertexBuffer->flush(device, vertexBufferSize, 0);
  // indexBuffer->flush(device, indexBufferSize, 0);
}

void UI::draw(VkCommandBuffer commandBuffer, int frameNum) {
  ImGuiIO& io = ImGui::GetIO();

  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          uiPipeline->getPipelineLayout(), 0, 1,
                          &uiDescriptorSets[frameNum], 0, nullptr);
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    uiPipeline->getPipeline());

  VkViewport viewport{};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = (float)ImGui::GetIO().DisplaySize.x;
  viewport.height = (float)ImGui::GetIO().DisplaySize.y;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

  PushConstBlock pushConstBlock = PushConstBlock();
  // UI scale and translate via push constants
  pushConstBlock.scale =
      glm::vec2(2.0f / io.DisplaySize.x, 2.0f / io.DisplaySize.y);
  pushConstBlock.translate = glm::vec2(-1.0f);
  vkCmdPushConstants(commandBuffer, uiPipeline->getPipelineLayout(),
                     VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                     0, sizeof(PushConstBlock), &pushConstBlock);

  // Render commands
  ImDrawData* imDrawData = ImGui::GetDrawData();
  int32_t vertexOffset = 0;
  int32_t indexOffset = 0;

  if (imDrawData->CmdListsCount > 0) {
    VkDeviceSize offsets[1] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer->getBuffer(),
                           offsets);
    vkCmdBindIndexBuffer(commandBuffer, indexBuffer->getBuffer(), 0,
                         VK_INDEX_TYPE_UINT16);

    for (int32_t i = 0; i < imDrawData->CmdListsCount; i++) {
      const ImDrawList* cmd_list = imDrawData->CmdLists[i];
      for (int32_t j = 0; j < cmd_list->CmdBuffer.Size; j++) {
        const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[j];
        VkRect2D scissorRect;
        scissorRect.offset.x = std::max((int32_t)(pcmd->ClipRect.x), 0);
        scissorRect.offset.y = std::max((int32_t)(pcmd->ClipRect.y), 0);
        scissorRect.extent.width =
            (uint32_t)(pcmd->ClipRect.z - pcmd->ClipRect.x);
        scissorRect.extent.height =
            (uint32_t)(pcmd->ClipRect.w - pcmd->ClipRect.y);
        vkCmdSetScissor(commandBuffer, 0, 1, &scissorRect);
        vkCmdDrawIndexed(commandBuffer, pcmd->ElemCount, 1, indexOffset,
                         vertexOffset, 0);
        indexOffset += pcmd->ElemCount;
      }
      vertexOffset += cmd_list->VtxBuffer.Size;
    }
  }
}