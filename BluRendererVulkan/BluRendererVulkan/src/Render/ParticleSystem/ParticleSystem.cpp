#include "ParticleSystem.h"

#include <random>

ParticleSystem::ParticleSystem(BaseRenderer* baseRenderer,
                               vks::VulkanDevice* device, VkQueue queue,
                               ParticleSystemInfo info) {
  this->device = device;
  this->info = info;

  textures.particle.loadFromFile(
      getAssetPath() + "textures/particle01_rgba.ktx", VK_FORMAT_R8G8B8A8_UNORM,
      device, queue);
  textures.gradient.loadFromFile(
      getAssetPath() + "textures/particle_gradient_rgba.ktx",
      VK_FORMAT_R8G8B8A8_UNORM, device, queue);


  prepareStorageBuffer(queue);
  prepareUniformBuffer();
  setupDescriptorPool();
  prepareGraphics(baseRenderer);
  prepareCompute(baseRenderer);
}

ParticleSystem::~ParticleSystem() {}

void ParticleSystem::setupDescriptorPool() {
  std::vector<VkDescriptorPoolSize> poolSizes = {
      vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                            1),
      vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                            1),
      vks::initializers::descriptorPoolSize(
          VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2)};
  VkDescriptorPoolCreateInfo descriptorPoolInfo =
      vks::initializers::descriptorPoolCreateInfo(poolSizes, 2);
  VK_CHECK_RESULT(vkCreateDescriptorPool(
      device->logicalDevice, &descriptorPoolInfo, nullptr, &descriptorPool));
}

void ParticleSystem::prepareUniformBuffer() {
  device->createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                           VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                       &computeUniformBuffer, sizeof(UniformData));
  VK_CHECK_RESULT(computeUniformBuffer.map());
  updateUniformBuffer(0);
}

void ParticleSystem::updateUniformBuffer(float frameTimer) {
  //uniformData.deltaT = frameTimer;

  //memcpy(computeUniformBuffer.mapped, &uniformData, sizeof(UniformData));
}

void ParticleSystem::prepareGraphics(BaseRenderer* br) {
  std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
      vks::initializers::descriptorSetLayoutBinding(
          VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
          VK_SHADER_STAGE_FRAGMENT_BIT, 0),
      vks::initializers::descriptorSetLayoutBinding(
          VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
          VK_SHADER_STAGE_FRAGMENT_BIT, 1)};
  VkDescriptorSetLayoutCreateInfo descriptorLayout =
      vks::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings);
  VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device->logicalDevice,
                                              &descriptorLayout, nullptr,
                                              &graphicsDescriptorSetLayout));

  VkDescriptorSetAllocateInfo allocInfo =
      vks::initializers::descriptorSetAllocateInfo(
          descriptorPool, &graphicsDescriptorSetLayout, 1);
  VK_CHECK_RESULT(vkAllocateDescriptorSets(device->logicalDevice, &allocInfo,
                                           &graphicsDescriptorSet));

  std::vector<VkWriteDescriptorSet> writeDescriptorSets{2};
  writeDescriptorSets[0] = vks::initializers::writeDescriptorSet(
      graphicsDescriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0,
      &textures.particle.descriptor);
  writeDescriptorSets[1] = vks::initializers::writeDescriptorSet(
      graphicsDescriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
      &textures.gradient.descriptor);

  vkUpdateDescriptorSets(device->logicalDevice,
                         static_cast<uint32_t>(writeDescriptorSets.size()),
                         writeDescriptorSets.data(), 0, NULL);

  VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo =
      vks::initializers::pipelineLayoutCreateInfo(&graphicsDescriptorSetLayout,
                                                  1);
  VK_CHECK_RESULT(vkCreatePipelineLayout(device->logicalDevice,
                                         &pipelineLayoutCreateInfo, nullptr,
                                         &graphicsPipelineLayout));

  VkPipelineInputAssemblyStateCreateInfo inputAssemblyState =
      vks::initializers::pipelineInputAssemblyStateCreateInfo(
          VK_PRIMITIVE_TOPOLOGY_POINT_LIST, 0, VK_FALSE);
  VkPipelineRasterizationStateCreateInfo rasterizationState =
      vks::initializers::pipelineRasterizationStateCreateInfo(
          VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE,
          VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);
  VkPipelineColorBlendAttachmentState blendAttachmentState =
      vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE);
  VkPipelineColorBlendStateCreateInfo colorBlendState =
      vks::initializers::pipelineColorBlendStateCreateInfo(
          1, &blendAttachmentState);
  VkPipelineDepthStencilStateCreateInfo depthStencilState =
      vks::initializers::pipelineDepthStencilStateCreateInfo(
          VK_FALSE, VK_FALSE, VK_COMPARE_OP_ALWAYS);
  VkPipelineViewportStateCreateInfo viewportState =
      vks::initializers::pipelineViewportStateCreateInfo(1, 1, 0);
  VkPipelineMultisampleStateCreateInfo multisampleState =
      vks::initializers::pipelineMultisampleStateCreateInfo(
          VK_SAMPLE_COUNT_1_BIT, 0);
  std::vector<VkDynamicState> dynamicStateEnables = {VK_DYNAMIC_STATE_VIEWPORT,
                                                     VK_DYNAMIC_STATE_SCISSOR};
  VkPipelineDynamicStateCreateInfo dynamicState =
      vks::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables);
  std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;

  std::vector<VkVertexInputBindingDescription> inputBindings = {
      vks::initializers::vertexInputBindingDescription(
          0, sizeof(Particle), VK_VERTEX_INPUT_RATE_VERTEX)};

  std::vector<VkVertexInputAttributeDescription> inputAttributes = {
      vks::initializers::vertexInputAttributeDescription(
          0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Particle, pos)),
      vks::initializers::vertexInputAttributeDescription(
          0, 1, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Particle, gradientPos)),
  };

  VkPipelineVertexInputStateCreateInfo vertexInputState =
      vks::initializers::pipelineVertexInputStateCreateInfo();
  vertexInputState.vertexBindingDescriptionCount =
      static_cast<uint32_t>(inputBindings.size());
  vertexInputState.pVertexBindingDescriptions = inputBindings.data();
  vertexInputState.vertexAttributeDescriptionCount =
      static_cast<uint32_t>(inputAttributes.size());
  vertexInputState.pVertexAttributeDescriptions = inputAttributes.data();

  shaderStages[0] = br->loadShader("shaders/computeparticles.vert.spv",
                                   VK_SHADER_STAGE_VERTEX_BIT);
  shaderStages[1] = br->loadShader("shaders/computeparticles.frag.spv",
                                   VK_SHADER_STAGE_FRAGMENT_BIT);

  VkGraphicsPipelineCreateInfo pipelineCreateInfo =
      vks::initializers::graphicsPipelineCreateInfo(graphicsPipelineLayout,
                                                    br->renderPass, 0);
  pipelineCreateInfo.pVertexInputState = &vertexInputState;
  pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
  pipelineCreateInfo.pRasterizationState = &rasterizationState;
  pipelineCreateInfo.pColorBlendState = &colorBlendState;
  pipelineCreateInfo.pMultisampleState = &multisampleState;
  pipelineCreateInfo.pViewportState = &viewportState;
  pipelineCreateInfo.pDepthStencilState = &depthStencilState;
  pipelineCreateInfo.pDynamicState = &dynamicState;
  pipelineCreateInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
  pipelineCreateInfo.pStages = shaderStages.data();
  pipelineCreateInfo.renderPass = br->renderPass;

  // Additive blending
  blendAttachmentState.colorWriteMask = 0xF;
  blendAttachmentState.blendEnable = VK_TRUE;
  blendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
  blendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
  blendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
  blendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
  blendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
  blendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_DST_ALPHA;
}

void ParticleSystem::prepareCompute(BaseRenderer* baseRenderer) {
  std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
      // Binding 0 : Particle position storage buffer
      vks::initializers::descriptorSetLayoutBinding(
          VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, 0),
      // Binding 1 : Uniform buffer
      vks::initializers::descriptorSetLayoutBinding(
          VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, 1),
  };
  VkDescriptorSetLayoutCreateInfo descriptorLayout =
      vks::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings);
  VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device->logicalDevice,
                                              &descriptorLayout, nullptr,
                                              &computeDescriptorSetLayout));

  VkDescriptorSetAllocateInfo allocInfo =
      vks::initializers::descriptorSetAllocateInfo(
          descriptorPool, &computeDescriptorSetLayout, 1);
  VK_CHECK_RESULT(vkAllocateDescriptorSets(device->logicalDevice, &allocInfo,
                                           &computeDescriptorSet));
  std::vector<VkWriteDescriptorSet> computeWriteDescriptorSets = {
      // Binding 0 : Particle position storage buffer
      vks::initializers::writeDescriptorSet(computeDescriptorSet,
                                            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                            0, &storageBuffer.descriptor),
      // Binding 1 : Uniform buffer
      vks::initializers::writeDescriptorSet(
          computeDescriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1,
          &computeUniformBuffer.descriptor)};
  vkUpdateDescriptorSets(
      device->logicalDevice,
      static_cast<uint32_t>(computeWriteDescriptorSets.size()),
      computeWriteDescriptorSets.data(), 0, NULL);

  // Create pipeline
  VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo =
      vks::initializers::pipelineLayoutCreateInfo(&computeDescriptorSetLayout,
                                                  1);
  VK_CHECK_RESULT(vkCreatePipelineLayout(device->logicalDevice,
                                         &pipelineLayoutCreateInfo, nullptr,
                                         &computePipelineLayout));
  VkComputePipelineCreateInfo computePipelineCreateInfo =
      vks::initializers::computePipelineCreateInfo(computePipelineLayout, 0);
  computePipelineCreateInfo.stage = baseRenderer->loadShader(
      "shaders/computeparticle.comp.spv", VK_SHADER_STAGE_COMPUTE_BIT);
  VK_CHECK_RESULT(vkCreateComputePipelines(device->logicalDevice, nullptr, 1,
                                           &computePipelineCreateInfo, nullptr,
                                           &computePipeline));

  // Semaphore for compute & graphics sync
  VkSemaphoreCreateInfo semaphoreCreateInfo =
      vks::initializers::semaphoreCreateInfo();
  VK_CHECK_RESULT(vkCreateSemaphore(device->logicalDevice, &semaphoreCreateInfo,
                                    nullptr, &computeSemaphore));
  //buildComputeBuffer();
}

//Builds & Dispatches Compute
void ParticleSystem::buildComputeBuffer(VkCommandBuffer cmdBuf, VkQueue queue) {
  // Compute Shader updating particle Vertices
  VkCommandBufferBeginInfo cmdBufInfo =
      vks::initializers::commandBufferBeginInfo();

  VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuf, &cmdBufInfo));

  // Add memory barrier to ensure that the (graphics) vertex shader has fetched
  // attributes before compute starts to write to the buffer
  if (device->queueFamilyIndices.graphics !=
      device->queueFamilyIndices.compute) {
    VkBufferMemoryBarrier buffer_barrier = {
        VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
        nullptr,
        0,
        VK_ACCESS_SHADER_WRITE_BIT,
        device->queueFamilyIndices.graphics,
        device->queueFamilyIndices.compute,
        storageBuffer.buffer,
        0,
        storageBuffer.size};

    vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 1,
                         &buffer_barrier, 0, nullptr);
  }

  vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline);
  vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_COMPUTE,
                          computePipelineLayout, 0, 1, &computeDescriptorSet, 0,
                          0);
  vkCmdDispatch(cmdBuf, info.particleCount / 256, 1, 1);

  // Add barrier to ensure that compute shader has finished writing to the
  // buffer Without this the (rendering) vertex shader may display incomplete
  // results (partial data from last frame)
  if (device->queueFamilyIndices.graphics !=
      device->queueFamilyIndices.compute) {
    VkBufferMemoryBarrier buffer_barrier = {
        VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
        nullptr,
        VK_ACCESS_SHADER_WRITE_BIT,
        0,
        device->queueFamilyIndices.compute,
        device->queueFamilyIndices.graphics,
        storageBuffer.buffer,
        0,
        storageBuffer.size};

    vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 1,
                         &buffer_barrier, 0, nullptr);
  }

  vkEndCommandBuffer(cmdBuf);

  // Wait for rendering finished
  VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

  // Submit compute commands
  VkSubmitInfo computeSubmitInfo = vks::initializers::submitInfo();
  computeSubmitInfo.commandBufferCount = 1;
  computeSubmitInfo.pCommandBuffers = &cmdBuf;
  computeSubmitInfo.waitSemaphoreCount = 1;
  computeSubmitInfo.pWaitSemaphores = &graphicsSemaphore;
  computeSubmitInfo.pWaitDstStageMask = &waitStageMask;
  computeSubmitInfo.signalSemaphoreCount = 1;
  computeSubmitInfo.pSignalSemaphores = &computeSemaphore;
  VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &computeSubmitInfo, VK_NULL_HANDLE));
}

void ParticleSystem::prepareStorageBuffer(VkQueue queue) {
  std::default_random_engine rndEngine((unsigned)time(nullptr));
  std::uniform_real_distribution<float> rndDist(-info.maxSpawnVariation,
                                                info.maxSpawnVariation);
  std::vector<Particle> particleBuffer(info.particleCount);
  for (auto& particle : particleBuffer) {
    particle.pos = glm::vec4(rndDist(rndEngine), rndDist(rndEngine),
                             rndDist(rndEngine), 0);
    particle.vel = glm::vec4(0.0f);
    particle.gradientPos.x = particle.pos.x / 2.0f;
  }

  VkDeviceSize storageBufferSize = particleBuffer.size() * sizeof(Particle);

  vks::Buffer stagingBuffer;
  device->createBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                           VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                       &stagingBuffer, storageBufferSize,
                       particleBuffer.data());

  device->createBuffer(
      // The SSBO will be used as a storage buffer for the compute pipeline and
      // as a vertex buffer in the graphics pipeline
      VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
          VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &storageBuffer, storageBufferSize);

  VkCommandBuffer copyCmd =
      device->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
  VkBufferCopy copyRegion = {};
  copyRegion.size = storageBufferSize;
  vkCmdCopyBuffer(copyCmd, stagingBuffer.buffer, storageBuffer.buffer, 1,
                  &copyRegion);
  // Execute a transfer barrier to the compute queue, if necessary
  if (device->queueFamilyIndices.graphics !=
      device->queueFamilyIndices.compute) {
    VkBufferMemoryBarrier buffer_barrier = {
        VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
        nullptr,
        VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
        0,
        device->queueFamilyIndices.graphics,
        device->queueFamilyIndices.compute,
        storageBuffer.buffer,
        0,
        storageBuffer.size};

    vkCmdPipelineBarrier(copyCmd, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
                         VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 1,
                         &buffer_barrier, 0, nullptr);
  }

  device->flushCommandBuffer(copyCmd, queue, true);

  stagingBuffer.destroy();
}

void ParticleSystem::update() {
  //if (vkGetFenceStatus()) {
  //}

  // Update particle update delta time
  // record/dispatch command buffer
  // dispatch compute buffer
  // repeat
}