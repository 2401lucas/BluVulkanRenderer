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
  prepareGraphics();
  prepareCompute(baseRenderer);
}

ParticleSystem::~ParticleSystem() {}

void ParticleSystem::draw(VkCommandBuffer cmdBuf) {
  // Wait for rendering finished
  VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

  // Submit compute commands
  VkSubmitInfo computeSubmitInfo = vks::initializers::submitInfo();
  computeSubmitInfo.commandBufferCount = 1;
  computeSubmitInfo.pCommandBuffers = &compute.commandBuffer;
  computeSubmitInfo.waitSemaphoreCount = 1;
  computeSubmitInfo.pWaitSemaphores = &graphics.semaphore;
  computeSubmitInfo.pWaitDstStageMask = &waitStageMask;
  computeSubmitInfo.signalSemaphoreCount = 1;
  computeSubmitInfo.pSignalSemaphores = &compute.semaphore;
  VK_CHECK_RESULT(
      vkQueueSubmit(compute.queue, 1, &computeSubmitInfo, VK_NULL_HANDLE));
}

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
                       &compute.uniformBuffer, sizeof(Compute::UniformData));
  VK_CHECK_RESULT(compute.uniformBuffer.map());
  updateUniformBuffer(0);
}

void ParticleSystem::updateUniformBuffer(float frameTimer) {
  compute.uniformData.deltaT = frameTimer;

  memcpy(compute.uniformBuffer.mapped, &compute.uniformData,
         sizeof(Compute::UniformData));
}

void ParticleSystem::prepareGraphics() {}

void ParticleSystem::buildCommandBuffer() {}

void ParticleSystem::prepareCompute(BaseRenderer* baseRenderer) {
  vkGetDeviceQueue(device->logicalDevice, device->queueFamilyIndices.compute, 0,
                   &compute.queue);

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
                                              &compute.descriptorSetLayout));

  VkDescriptorSetAllocateInfo allocInfo =
      vks::initializers::descriptorSetAllocateInfo(
          descriptorPool, &compute.descriptorSetLayout, 1);
  VK_CHECK_RESULT(vkAllocateDescriptorSets(device->logicalDevice, &allocInfo,
                                           &compute.descriptorSet));
  std::vector<VkWriteDescriptorSet> computeWriteDescriptorSets = {
      // Binding 0 : Particle position storage buffer
      vks::initializers::writeDescriptorSet(compute.descriptorSet,
                                            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                            0, &storageBuffer.descriptor),
      // Binding 1 : Uniform buffer
      vks::initializers::writeDescriptorSet(
          compute.descriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1,
          &compute.uniformBuffer.descriptor)};
  vkUpdateDescriptorSets(
      device->logicalDevice,
      static_cast<uint32_t>(computeWriteDescriptorSets.size()),
      computeWriteDescriptorSets.data(), 0, NULL);

  // Create pipeline
  VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo =
      vks::initializers::pipelineLayoutCreateInfo(&compute.descriptorSetLayout,
                                                  1);
  VK_CHECK_RESULT(vkCreatePipelineLayout(device->logicalDevice,
                                         &pipelineLayoutCreateInfo, nullptr,
                                         &compute.pipelineLayout));
  VkComputePipelineCreateInfo computePipelineCreateInfo =
      vks::initializers::computePipelineCreateInfo(compute.pipelineLayout, 0);
  computePipelineCreateInfo.stage =
      baseRenderer->loadShader("shaders/computeparticles/particle.comp.spv",
                               VK_SHADER_STAGE_COMPUTE_BIT);
  VK_CHECK_RESULT(vkCreateComputePipelines(device->logicalDevice, nullptr, 1,
                                           &computePipelineCreateInfo, nullptr,
                                           &compute.pipeline));

  // Separate command pool as queue family for compute may be different than
  // graphics
  VkCommandPoolCreateInfo cmdPoolInfo = {};
  cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  cmdPoolInfo.queueFamilyIndex = device->queueFamilyIndices.compute;
  cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  VK_CHECK_RESULT(vkCreateCommandPool(device->logicalDevice, &cmdPoolInfo,
                                      nullptr, &compute.commandPool));

  // Create a command buffer for compute operations
  compute.commandBuffer = device->createCommandBuffer(
      VK_COMMAND_BUFFER_LEVEL_PRIMARY, compute.commandPool);

  // Semaphore for compute & graphics sync
  VkSemaphoreCreateInfo semaphoreCreateInfo =
      vks::initializers::semaphoreCreateInfo();
  VK_CHECK_RESULT(vkCreateSemaphore(device->logicalDevice, &semaphoreCreateInfo,
                                    nullptr, &compute.semaphore));
  buildComputeBuffer();
}

void ParticleSystem::buildComputeBuffer() {
  // Compute Shader updating particle Vertices
  VkCommandBufferBeginInfo cmdBufInfo =
      vks::initializers::commandBufferBeginInfo();

  VK_CHECK_RESULT(vkBeginCommandBuffer(compute.commandBuffer, &cmdBufInfo));

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

    vkCmdPipelineBarrier(compute.commandBuffer,
                         VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 1,
                         &buffer_barrier, 0, nullptr);
  }

  vkCmdBindPipeline(compute.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                    compute.pipeline);
  vkCmdBindDescriptorSets(compute.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                          compute.pipelineLayout, 0, 1, &compute.descriptorSet,
                          0, 0);
  vkCmdDispatch(compute.commandBuffer, info.particleCount / 256, 1, 1);

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

    vkCmdPipelineBarrier(compute.commandBuffer,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 1,
                         &buffer_barrier, 0, nullptr);
  }

  vkEndCommandBuffer(compute.commandBuffer);
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


void ParticleSystem::update() {}