#include "vkImGui.h"

vkImGUI::vkImGUI(BaseRenderer* br) : baseRenderer(br) {
  device = br->vulkanDevice;
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.FontGlobalScale = 1;
  ImGuiStyle& style = ImGui::GetStyle();
  style.ScaleAllSizes(1);
};

vkImGUI::~vkImGUI() {
  ImGui::DestroyContext();
  for (auto& buf : vertexBuffers) {
    buf.destroy();
  }
  for (auto& buf : indexBuffers) {
    buf.destroy();
  }
  vkDestroyImage(device->logicalDevice, fontImage, nullptr);
  vkDestroyImageView(device->logicalDevice, fontView, nullptr);
  vkFreeMemory(device->logicalDevice, fontMemory, nullptr);
  vkDestroySampler(device->logicalDevice, sampler, nullptr);
  vkDestroyPipelineCache(device->logicalDevice, pipelineCache, nullptr);
  vkDestroyPipeline(device->logicalDevice, pipeline, nullptr);
  vkDestroyPipelineLayout(device->logicalDevice, pipelineLayout, nullptr);
  vkDestroyDescriptorPool(device->logicalDevice, descriptorPool, nullptr);
  vkDestroyDescriptorSetLayout(device->logicalDevice, descriptorSetLayout,
                               nullptr);
}

void vkImGUI::init(float width, float height) {
  // Color scheme
  vulkanStyle = ImGui::GetStyle();
  vulkanStyle.Colors[ImGuiCol_TitleBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.6f);
  vulkanStyle.Colors[ImGuiCol_TitleBgActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);
  vulkanStyle.Colors[ImGuiCol_MenuBarBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
  vulkanStyle.Colors[ImGuiCol_Header] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
  vulkanStyle.Colors[ImGuiCol_CheckMark] = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);

  setStyle(0);

  // Dimensions
  ImGuiIO& io = ImGui::GetIO();
  io.DisplaySize = ImVec2(width, height);
  io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
  // io.KeyMap[ImGuiKey_Tab] = VK_TAB;
  // io.KeyMap[ImGuiKey_LeftArrow] = VK_LEFT;
  // io.KeyMap[ImGuiKey_RightArrow] = VK_RIGHT;
  // io.KeyMap[ImGuiKey_UpArrow] = VK_UP;
  // io.KeyMap[ImGuiKey_DownArrow] = VK_DOWN;
  // io.KeyMap[ImGuiKey_Backspace] = VK_BACK;
  // io.KeyMap[ImGuiKey_Enter] = VK_RETURN;
  // io.KeyMap[ImGuiKey_Space] = VK_SPACE;
  // io.KeyMap[ImGuiKey_Delete] = VK_DELETE;
  // io.KeyMap[ImGuiKey_0] = GLFW_KEY_0;
  // io.KeyMap[ImGuiKey_1] = GLFW_KEY_1;
  // io.KeyMap[ImGuiKey_2] = GLFW_KEY_2;
  // io.KeyMap[ImGuiKey_3] = GLFW_KEY_3;
  // io.KeyMap[ImGuiKey_4] = GLFW_KEY_4;
  // io.KeyMap[ImGuiKey_5] = GLFW_KEY_5;
  // io.KeyMap[ImGuiKey_6] = GLFW_KEY_6;
  // io.KeyMap[ImGuiKey_7] = GLFW_KEY_7;
  // io.KeyMap[ImGuiKey_8] = GLFW_KEY_8;
  // io.KeyMap[ImGuiKey_9] = GLFW_KEY_9;
  // io.AddKeyEvent(GLFW_KEY_0);
  // io.AddInputCharacter(GLFW_KEY_1);
  // io.AddInputCharacter(GLFW_KEY_2);
  // io.AddInputCharacter(GLFW_KEY_3);
  // io.AddInputCharacter(GLFW_KEY_4);
  // io.AddInputCharacter(GLFW_KEY_5);
  // io.AddInputCharacter(GLFW_KEY_6);
  // io.AddInputCharacter(GLFW_KEY_7);
  // io.AddInputCharacter(GLFW_KEY_8);
  // io.AddInputCharacter(GLFW_KEY_9);

  io.AddKeyEvent(ImGuiKey_0, true);
  io.AddKeyEvent(ImGuiKey_1, true);
  io.AddKeyEvent(ImGuiKey_2, true);
  io.AddKeyEvent(ImGuiKey_3, true);
  io.AddKeyEvent(ImGuiKey_4, true);
  io.AddKeyEvent(ImGuiKey_5, true);
  io.AddKeyEvent(ImGuiKey_6, true);
  io.AddKeyEvent(ImGuiKey_7, true);
  io.AddKeyEvent(ImGuiKey_8, true);
  io.AddKeyEvent(ImGuiKey_9, true);
}

void vkImGUI::setStyle(uint32_t index) {
  selectedStyle = index;
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

void vkImGUI::initResources(BaseRenderer* br, VkQueue copyQueue,
                            const std::string& shadersPath) {
  ImGuiIO& io = ImGui::GetIO();
  // Create font texture
  unsigned char* fontData;
  int texWidth, texHeight;
  io.Fonts->GetTexDataAsRGBA32(&fontData, &texWidth, &texHeight);
  VkDeviceSize uploadSize = texWidth * texHeight * 4 * sizeof(char);

  if (device->extensionSupported(VK_KHR_DRIVER_PROPERTIES_EXTENSION_NAME)) {
    VkPhysicalDeviceProperties2 deviceProperties2 = {};
    deviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    deviceProperties2.pNext = &driverProperties;
    driverProperties.sType =
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES;
    vkGetPhysicalDeviceProperties2(device->physicalDevice, &deviceProperties2);
  }

  vertexBuffers.resize(br->swapChain.imageCount);
  vertexCounts.resize(br->swapChain.imageCount);
  indexBuffers.resize(br->swapChain.imageCount);
  indexCounts.resize(br->swapChain.imageCount);

  // Create target image for copy
  VkImageCreateInfo imageInfo = vks::initializers::imageCreateInfo();
  imageInfo.imageType = VK_IMAGE_TYPE_2D;
  imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
  imageInfo.extent.width = texWidth;
  imageInfo.extent.height = texHeight;
  imageInfo.extent.depth = 1;
  imageInfo.mipLevels = 1;
  imageInfo.arrayLayers = 1;
  imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  imageInfo.usage =
      VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  VK_CHECK_RESULT(
      vkCreateImage(device->logicalDevice, &imageInfo, nullptr, &fontImage));
  VkMemoryRequirements memReqs;
  vkGetImageMemoryRequirements(device->logicalDevice, fontImage, &memReqs);
  VkMemoryAllocateInfo memAllocInfo = vks::initializers::memoryAllocateInfo();
  memAllocInfo.allocationSize = memReqs.size;
  memAllocInfo.memoryTypeIndex = device->getMemoryType(
      memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  VK_CHECK_RESULT(vkAllocateMemory(device->logicalDevice, &memAllocInfo,
                                   nullptr, &fontMemory));
  VK_CHECK_RESULT(
      vkBindImageMemory(device->logicalDevice, fontImage, fontMemory, 0));

  // Image view
  VkImageViewCreateInfo viewInfo = vks::initializers::imageViewCreateInfo();
  viewInfo.image = fontImage;
  viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
  viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  viewInfo.subresourceRange.levelCount = 1;
  viewInfo.subresourceRange.layerCount = 1;
  VK_CHECK_RESULT(
      vkCreateImageView(device->logicalDevice, &viewInfo, nullptr, &fontView));

  // Staging buffers for font data upload
  vks::Buffer stagingBuffer;

  VK_CHECK_RESULT(device->createBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                           VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                       &stagingBuffer, uploadSize));

  stagingBuffer.map();
  memcpy(stagingBuffer.mapped, fontData, uploadSize);
  stagingBuffer.unmap();

  // Copy buffer data to font image
  VkCommandBuffer copyCmd =
      device->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

  // Prepare for transfer
  vks::tools::setImageLayout(
      copyCmd, fontImage, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_HOST_BIT,
      VK_PIPELINE_STAGE_TRANSFER_BIT);

  // Copy
  VkBufferImageCopy bufferCopyRegion = {};
  bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  bufferCopyRegion.imageSubresource.layerCount = 1;
  bufferCopyRegion.imageExtent.width = texWidth;
  bufferCopyRegion.imageExtent.height = texHeight;
  bufferCopyRegion.imageExtent.depth = 1;

  vkCmdCopyBufferToImage(copyCmd, stagingBuffer.buffer, fontImage,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                         &bufferCopyRegion);

  // Prepare for shader read
  vks::tools::setImageLayout(copyCmd, fontImage, VK_IMAGE_ASPECT_COLOR_BIT,
                             VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                             VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

  device->flushCommandBuffer(copyCmd, copyQueue, true);

  stagingBuffer.destroy();

  // Font texture Sampler
  VkSamplerCreateInfo samplerInfo = vks::initializers::samplerCreateInfo();
  samplerInfo.magFilter = VK_FILTER_LINEAR;
  samplerInfo.minFilter = VK_FILTER_LINEAR;
  samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
  VK_CHECK_RESULT(
      vkCreateSampler(device->logicalDevice, &samplerInfo, nullptr, &sampler));

  // Descriptor pool
  std::vector<VkDescriptorPoolSize> poolSizes = {
      vks::initializers::descriptorPoolSize(
          VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1)};
  VkDescriptorPoolCreateInfo descriptorPoolInfo =
      vks::initializers::descriptorPoolCreateInfo(poolSizes, 2);
  VK_CHECK_RESULT(vkCreateDescriptorPool(
      device->logicalDevice, &descriptorPoolInfo, nullptr, &descriptorPool));

  // Descriptor set layout
  std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
      vks::initializers::descriptorSetLayoutBinding(
          VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
          VK_SHADER_STAGE_FRAGMENT_BIT, 0),
  };
  VkDescriptorSetLayoutCreateInfo descriptorLayout =
      vks::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings);
  VK_CHECK_RESULT(vkCreateDescriptorSetLayout(
      device->logicalDevice, &descriptorLayout, nullptr, &descriptorSetLayout));

  // Descriptor set
  VkDescriptorSetAllocateInfo allocInfo =
      vks::initializers::descriptorSetAllocateInfo(descriptorPool,
                                                   &descriptorSetLayout, 1);
  VK_CHECK_RESULT(vkAllocateDescriptorSets(device->logicalDevice, &allocInfo,
                                           &descriptorSet));
  VkDescriptorImageInfo fontDescriptor = vks::initializers::descriptorImageInfo(
      sampler, fontView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
  std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
      vks::initializers::writeDescriptorSet(
          descriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0,
          &fontDescriptor)};
  vkUpdateDescriptorSets(device->logicalDevice,
                         static_cast<uint32_t>(writeDescriptorSets.size()),
                         writeDescriptorSets.data(), 0, nullptr);

  // Pipeline cache
  VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
  pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
  VK_CHECK_RESULT(vkCreatePipelineCache(device->logicalDevice,
                                        &pipelineCacheCreateInfo, nullptr,
                                        &pipelineCache));

  // Pipeline layout
  // Push constants for UI rendering parameters
  VkPushConstantRange pushConstantRange = vks::initializers::pushConstantRange(
      VK_SHADER_STAGE_VERTEX_BIT, sizeof(PushConstBlock), 0);
  VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo =
      vks::initializers::pipelineLayoutCreateInfo(&descriptorSetLayout, 1);
  pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
  pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;
  VK_CHECK_RESULT(vkCreatePipelineLayout(device->logicalDevice,
                                         &pipelineLayoutCreateInfo, nullptr,
                                         &pipelineLayout));

  // Setup graphics pipeline for UI rendering
  VkPipelineInputAssemblyStateCreateInfo inputAssemblyState =
      vks::initializers::pipelineInputAssemblyStateCreateInfo(
          VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);

  VkPipelineRasterizationStateCreateInfo rasterizationState =
      vks::initializers::pipelineRasterizationStateCreateInfo(
          VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE,
          VK_FRONT_FACE_COUNTER_CLOCKWISE);

  // Enable blending
  VkPipelineColorBlendAttachmentState blendAttachmentState{};
  blendAttachmentState.blendEnable = VK_TRUE;
  blendAttachmentState.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  blendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
  blendAttachmentState.dstColorBlendFactor =
      VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  blendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
  blendAttachmentState.srcAlphaBlendFactor =
      VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  blendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  blendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;

  VkPipelineColorBlendStateCreateInfo colorBlendState =
      vks::initializers::pipelineColorBlendStateCreateInfo(
          1, &blendAttachmentState);

  VkPipelineDepthStencilStateCreateInfo depthStencilState =
      vks::initializers::pipelineDepthStencilStateCreateInfo(
          VK_FALSE, VK_FALSE, VK_COMPARE_OP_LESS_OR_EQUAL);

  VkPipelineViewportStateCreateInfo viewportState =
      vks::initializers::pipelineViewportStateCreateInfo(1, 1, 0);

  // TODO: Recreate Pipeline when multisample state changes
  VkPipelineMultisampleStateCreateInfo multisampleState =
      vks::initializers::pipelineMultisampleStateCreateInfo(
          br->getMSAASampleCount());

  std::vector<VkDynamicState> dynamicStateEnables = {VK_DYNAMIC_STATE_VIEWPORT,
                                                     VK_DYNAMIC_STATE_SCISSOR};
  VkPipelineDynamicStateCreateInfo dynamicState =
      vks::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables);

  std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages{};

  VkGraphicsPipelineCreateInfo pipelineCreateInfo =
      vks::initializers::graphicsPipelineCreateInfo(pipelineLayout,
                                                    br->renderPass);

  pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
  pipelineCreateInfo.pRasterizationState = &rasterizationState;
  pipelineCreateInfo.pColorBlendState = &colorBlendState;
  pipelineCreateInfo.pMultisampleState = &multisampleState;
  pipelineCreateInfo.pViewportState = &viewportState;
  pipelineCreateInfo.pDepthStencilState = &depthStencilState;
  pipelineCreateInfo.pDynamicState = &dynamicState;
  pipelineCreateInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
  pipelineCreateInfo.pStages = shaderStages.data();

  // Vertex bindings an attributes based on ImGui vertex definition
  std::vector<VkVertexInputBindingDescription> vertexInputBindings = {
      vks::initializers::vertexInputBindingDescription(
          0, sizeof(ImDrawVert), VK_VERTEX_INPUT_RATE_VERTEX),
  };
  std::vector<VkVertexInputAttributeDescription> vertexInputAttributes = {
      vks::initializers::vertexInputAttributeDescription(
          0, 0, VK_FORMAT_R32G32_SFLOAT,
          offsetof(ImDrawVert, pos)),  // Location 0: Position
      vks::initializers::vertexInputAttributeDescription(
          0, 1, VK_FORMAT_R32G32_SFLOAT,
          offsetof(ImDrawVert, uv)),  // Location 1: UV
      vks::initializers::vertexInputAttributeDescription(
          0, 2, VK_FORMAT_R8G8B8A8_UNORM,
          offsetof(ImDrawVert, col)),  // Location 0: Color
  };
  VkPipelineVertexInputStateCreateInfo vertexInputState =
      vks::initializers::pipelineVertexInputStateCreateInfo();
  vertexInputState.vertexBindingDescriptionCount =
      static_cast<uint32_t>(vertexInputBindings.size());
  vertexInputState.pVertexBindingDescriptions = vertexInputBindings.data();
  vertexInputState.vertexAttributeDescriptionCount =
      static_cast<uint32_t>(vertexInputAttributes.size());
  vertexInputState.pVertexAttributeDescriptions = vertexInputAttributes.data();

  pipelineCreateInfo.pVertexInputState = &vertexInputState;

  shaderStages[0] = baseRenderer->loadShader("shaders/ui.vert.spv",
                                             VK_SHADER_STAGE_VERTEX_BIT);
  shaderStages[1] = baseRenderer->loadShader("shaders/ui.frag.spv",
                                             VK_SHADER_STAGE_FRAGMENT_BIT);

  VK_CHECK_RESULT(
      vkCreateGraphicsPipelines(device->logicalDevice, pipelineCache, 1,
                                &pipelineCreateInfo, nullptr, &pipeline));
}

// Update vertex and index buffer containing the imGui elements when required

void vkImGUI::updateBuffers(uint32_t frameIndex) {
  ImDrawData* imDrawData = ImGui::GetDrawData();

  // Note: Alignment is done inside buffer creation
  VkDeviceSize vertexBufferSize =
      imDrawData->TotalVtxCount * sizeof(ImDrawVert);
  VkDeviceSize indexBufferSize = imDrawData->TotalIdxCount * sizeof(ImDrawIdx);

  if ((vertexBufferSize == 0) || (indexBufferSize == 0)) {
    return;
  }

  // Vertex buffer
  if ((vertexBuffers[frameIndex].buffer == VK_NULL_HANDLE) ||
      (vertexCounts[frameIndex] != imDrawData->TotalVtxCount)) {
    vertexBuffers[frameIndex].unmap();
    vertexBuffers[frameIndex].destroy();
    VK_CHECK_RESULT(device->createBuffer(
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        &vertexBuffers[frameIndex], vertexBufferSize));
    vertexCounts[frameIndex] = imDrawData->TotalVtxCount;
    vertexBuffers[frameIndex].map();
  }

  // Index buffer
  if ((indexBuffers[frameIndex].buffer == VK_NULL_HANDLE) ||
      (indexCounts[frameIndex] < imDrawData->TotalIdxCount)) {
    indexBuffers[frameIndex].unmap();
    indexBuffers[frameIndex].destroy();
    VK_CHECK_RESULT(device->createBuffer(
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        &indexBuffers[frameIndex], indexBufferSize));
    indexCounts[frameIndex] = imDrawData->TotalIdxCount;
    indexBuffers[frameIndex].map();
  }

  // Upload data
  ImDrawVert* vtxDst = (ImDrawVert*)vertexBuffers[frameIndex].mapped;
  ImDrawIdx* idxDst = (ImDrawIdx*)indexBuffers[frameIndex].mapped;

  for (int n = 0; n < imDrawData->CmdListsCount; n++) {
    const ImDrawList* cmd_list = imDrawData->CmdLists[n];
    memcpy(vtxDst, cmd_list->VtxBuffer.Data,
           cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
    memcpy(idxDst, cmd_list->IdxBuffer.Data,
           cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
    vtxDst += cmd_list->VtxBuffer.Size;
    idxDst += cmd_list->IdxBuffer.Size;
  }

  // Flush to make writes visible to GPU
  vertexBuffers[frameIndex].flush();
  indexBuffers[frameIndex].flush();
}

// Draw current imGui frame into a command buffer
void vkImGUI::drawFrame(VkCommandBuffer commandBuffer, uint32_t frameIndex) {
  ImGuiIO& io = ImGui::GetIO();

  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

  VkViewport viewport = vks::initializers::viewport(
      ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y, 0.0f, 1.0f);
  vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

  // UI scale and translate via push constants
  pushConstBlock.scale =
      glm::vec2(2.0f / io.DisplaySize.x, 2.0f / io.DisplaySize.y);
  pushConstBlock.translate = glm::vec2(-1.0f);
  vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
                     0, sizeof(PushConstBlock), &pushConstBlock);

  // Render commands
  ImDrawData* imDrawData = ImGui::GetDrawData();
  int32_t vertexOffset = 0;
  int32_t indexOffset = 0;

  if (imDrawData->CmdListsCount > 0) {
    VkDeviceSize offsets[1] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1,
                           &vertexBuffers[frameIndex].buffer, offsets);
    vkCmdBindIndexBuffer(commandBuffer, indexBuffers[frameIndex].buffer, 0,
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