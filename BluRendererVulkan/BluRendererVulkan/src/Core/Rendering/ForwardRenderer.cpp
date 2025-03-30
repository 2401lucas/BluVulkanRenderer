#include "ForwardRenderer.h"

#include <cassert>

#include "../External/FileManager.h"
#include "Vulkan/Tools.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../External/stb_image.h"

ForwardRenderer::ForwardRenderer(blu::core::Window* window) {
  window_ = window;
  // VkInstance Creation
  {
    eastl::vector<eastl::string> instance_extensions = {
        VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
    };

    instance_ = new blu::core::Instance("Forward Renderer", USE_VALIDATION,
                                        instance_extensions);
  }
  // VkDevice Creation
  {
    VkPhysicalDeviceFeatures physical_device_requested_features_{
        .multiDrawIndirect = VK_TRUE,
        .wideLines = VK_TRUE,
        .samplerAnisotropy = VK_TRUE,
        .shaderInt64 = VK_TRUE,
    };

    eastl::vector<const char*> device_extensions = {
        VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
        VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
        VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
        VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
    };

    VkPhysicalDeviceDynamicRenderingFeaturesKHR dynamic_rendering{
        .sType =
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR,
        .dynamicRendering = VK_TRUE,
    };
    void* p_next_chain = &dynamic_rendering;

    VkPhysicalDeviceDescriptorIndexingFeaturesEXT descriptor_indexing{
        .sType =
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT,
        .descriptorBindingPartiallyBound = VK_TRUE,
        .runtimeDescriptorArray = VK_TRUE,
    };
    dynamic_rendering.pNext = &descriptor_indexing;

    VkPhysicalDeviceBufferDeviceAddressFeaturesKHR buffer_device_address{
        .sType =
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES_KHR,
        .bufferDeviceAddress = VK_TRUE,
    };
    descriptor_indexing.pNext = &buffer_device_address;

    VkPhysicalDeviceSynchronization2FeaturesKHR device_sync{
        .sType =
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES_KHR,
        .synchronization2 = VK_TRUE,
    };
    buffer_device_address.pNext = &device_sync;

    device_ =
        new blu::core::Device(instance_, physical_device_requested_features_,
                              device_extensions, p_next_chain);
  }
  // VkSwapchain Creation
  {
    swapchain_ = new blu::core::Swapchain(instance_, device_, window_);
    swapchain_->Create(false, false);
  }
  // VMA initialization
  {
    VmaVulkanFunctions vulkan_functions{
        .vkGetInstanceProcAddr = &vkGetInstanceProcAddr,
        .vkGetDeviceProcAddr = &vkGetDeviceProcAddr,
    };

    VmaAllocatorCreateInfo allocator_create_info{
        .flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
        .physicalDevice = device_->GetPhysicalDevice(),
        .device = device_->GetLogicalDevice(),
        .pVulkanFunctions = &vulkan_functions,
        .instance = instance_->Get(),
        .vulkanApiVersion = VK_API_VERSION_1_3,
    };

    vmaCreateAllocator(&allocator_create_info, &allocator_);
  }
}

ForwardRenderer::~ForwardRenderer() {
  vkDeviceWaitIdle(device_->GetLogicalDevice());
  for (size_t i = 0; i < swapchain_->GetImageCount(); i++) {
    vkDestroyCommandPool(device_->GetLogicalDevice(),
                         graphics_command_pools_[i], nullptr);
    vkDestroyCommandPool(device_->GetLogicalDevice(), transfer_command_pools[i],
                         nullptr);
    vkDestroyCommandPool(device_->GetLogicalDevice(), compute_command_pools_[i],
                         nullptr);
    vkDestroySemaphore(device_->GetLogicalDevice(),
                       image_available_semaphores_[i], nullptr);
    vkDestroySemaphore(device_->GetLogicalDevice(),
                       render_finished_semaphores_[i], nullptr);
    vkDestroySemaphore(device_->GetLogicalDevice(), dcg_semaphores_[i],
                       nullptr);
    vkDestroyFence(device_->GetLogicalDevice(), in_flight_fences_[i], nullptr);
  }

  vkDestroyDescriptorPool(device_->GetLogicalDevice(), render_descriptor_pool_,
                          nullptr);
  depth_stencil_image_->Destroy(device_->GetLogicalDevice(), allocator_);
  delete depth_stencil_image_;
  buffer_infos_buffer_->Destroy(allocator_);
  delete buffer_infos_buffer_;
  buffer_infos_descriptor_set_->Destroy(device_->GetLogicalDevice());
  delete buffer_infos_descriptor_set_;
  textures_descriptor_set_->Destroy(device_->GetLogicalDevice());
  delete textures_descriptor_set_;
  matrices_buffer_->Destroy(allocator_);
  delete matrices_buffer_;

  vertex_buffer_->Destroy(allocator_);
  delete vertex_buffer_;
  normal_buffer_->Destroy(allocator_);
  delete normal_buffer_;
  index_buffer_->Destroy(allocator_);
  delete index_buffer_;
  uv_buffer_->Destroy(allocator_);
  delete uv_buffer_;

  for (auto& tex : textures) {
    tex->Destroy(device_->GetLogicalDevice(), allocator_);
    delete tex;
  }
  for (auto& buf : dcg_input_model_data_) {
    buf->Destroy(allocator_);
    delete buf;
  }
  for (auto& buf : dcg_input_models_) {
    buf->Destroy(allocator_);
    delete buf;
  }
  for (auto& buf : dcg_output_buffers_) {
    buf->Destroy(allocator_);
    delete buf;
  }

  for (eastl::vector<blu::core::components::Model>::iterator
           it = loaded_models_.begin(),
           it_end = loaded_models_.end();
       it != it_end; ++it) {
    it->Delete();
  }

  for (eastl::vector<VkShaderModule>::iterator it = shader_modules_.begin(),
                                               it_end = shader_modules_.end();
       it != it_end; ++it) {
    vkDestroyShaderModule(device_->GetLogicalDevice(), *it, nullptr);
  }

  delete dcg_pipeline_;
  delete triangle_pipeline_;
  delete cube_pipeline_;

  vmaDestroyAllocator(allocator_);
  delete swapchain_;
  delete device_;
  delete instance_;
}

// For now- NOT ASYNC
int ForwardRenderer::LoadModel(eastl::string filepath) {
  if (loaded_model_indices_.find(filepath) != loaded_model_indices_.end()) {
    return loaded_model_indices_[filepath];
  }
  uint32_t model_index = loaded_models_.size();
  ModelIndices model_index_data{};
  // Texturing Data
  {
    // PBR MATERIAL
    if (blu::core::file::DoesFileExist(filepath + "_BaseColor.png") &&
        blu::core::file::DoesFileExist(filepath + "_MetallicRoughness.png")) {
      model_index_data.material_type = METALLIC_ROUGHNESS;
      model_index_data.main_tex_id = LoadImage(filepath + "_BaseColor.png");
      model_index_data.secondary_tex_id =
          LoadImage(filepath + "_MetallicRoughness.png");
      model_index_data.tertiary_tex_id = -1;
    }
  }

  // Model Data
  {
    auto new_model = blu::core::components::Model(filepath + ".glTF");

    if (new_model.GetVertexData() == nullptr) {
      return -1;
    }

    loaded_models_.push_back(new_model);
    auto& model = loaded_models_[model_index];
    loaded_model_indices_[filepath] = model_index;

    VkCommandBufferAllocateInfo alloc_info{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = transfer_command_pools[frame_index_],
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };

    VkCommandBuffer copy_cmd_buf;
    vkAllocateCommandBuffers(device_->GetLogicalDevice(), &alloc_info,
                             &copy_cmd_buf);

    VkCommandBufferBeginInfo begin_info{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    vkBeginCommandBuffer(copy_cmd_buf, &begin_info);

    blu::core::Buffer* vert_staging_buffer;
    blu::core::Buffer::UploadToBuffer(
        device_->GetLogicalDevice(), allocator_, vertex_buffer_,
        vertex_buffer_offset_, copy_cmd_buf, model.GetVertexData(),
        model.GetVertexDataSize(), 0, vert_staging_buffer);
    blu::core::Buffer* normal_staging_buffer;
    blu::core::Buffer::UploadToBuffer(
        device_->GetLogicalDevice(), allocator_, normal_buffer_,
        normal_buffer_offset_, copy_cmd_buf, model.GetNormalData(),
        model.GetNormalDataSize(), 0, normal_staging_buffer);
    blu::core::Buffer* index_staging_buffer;
    blu::core::Buffer::UploadToBuffer(
        device_->GetLogicalDevice(), allocator_, index_buffer_,
        index_buffer_offset_, copy_cmd_buf, model.GetIndexData(),
        model.GetIndexDataSize(), 0, index_staging_buffer);
    blu::core::Buffer* uv_staging_buffer;
    blu::core::Buffer::UploadToBuffer(
        device_->GetLogicalDevice(), allocator_, uv_buffer_, uv_buffer_offset_,
        copy_cmd_buf, model.GetUVData(), model.GetUVDataSize(), 0,
        uv_staging_buffer);

    vkEndCommandBuffer(copy_cmd_buf);

    VkSubmitInfo submitInfo{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &copy_cmd_buf,
    };

    vkQueueSubmit(device_->queues.transfer, 1, &submitInfo, VK_NULL_HANDLE);

    model_index_data.vert_offset = vertex_buffer_offset_;
    model_index_data.ind_count = model.GetIndexCount();
    model_index_data.ind_offset = index_buffer_offset_;

    vertex_buffer_offset_ += sizeof(float) * 3 * model.GetVertexCount();
    normal_buffer_offset_ += sizeof(float) * 3 * model.GetVertexCount();
    index_buffer_offset_ += sizeof(uint32_t) * model.GetIndexCount();
    uv_buffer_offset_ += sizeof(float) * 3 * model.GetVertexCount();

    // TODO: REMOVE
    vkDeviceWaitIdle(device_->GetLogicalDevice());

    vkFreeCommandBuffers(device_->GetLogicalDevice(),
                         transfer_command_pools[frame_index_], 1,
                         &copy_cmd_buf);

    vert_staging_buffer->Destroy(allocator_);
    delete vert_staging_buffer;
    normal_staging_buffer->Destroy(allocator_);
    delete normal_staging_buffer;
    index_staging_buffer->Destroy(allocator_);
    delete index_staging_buffer;
    uv_staging_buffer->Destroy(allocator_);
    delete uv_staging_buffer;
  }

  model_indices_.push_back(model_index_data);

  return model_index;
}

// TODO: Support More Image Compositions / Filetypes (At LoadImage Call, maybe
// make it file ending agnostic?)
int ForwardRenderer::LoadImage(eastl::string filepath) {
  if (loaded_texture_indices_.find(filepath) != loaded_texture_indices_.end()) {
    return loaded_texture_indices_[filepath];
  }

  int width, height, channels;
  unsigned char* image_data =
      stbi_load(filepath.c_str(), &width, &height, &channels, STBI_rgb_alpha);
  if (!image_data) {
    std::cerr << "Failed to load image!" << std::endl;
    return -1;
  }

  VkCommandBufferAllocateInfo alloc_info{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool = graphics_command_pools_[frame_index_],
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = 1,
  };

  VkCommandBuffer copy_cmd_buf;
  vkAllocateCommandBuffers(device_->GetLogicalDevice(), &alloc_info,
                           &copy_cmd_buf);

  VkCommandBufferBeginInfo begin_info{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
  };
  vkBeginCommandBuffer(copy_cmd_buf, &begin_info);

  blu::core::Buffer* img_staging_buffer;
  auto new_image = blu::core::Image::CreateImage(
      device_->GetLogicalDevice(), allocator_, copy_cmd_buf,
      device_->queue_family_indicies_.graphics,
      device_->queue_family_indicies_.graphics, COLOR_FORMAT, width, height,
      VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TILING_OPTIMAL,
      VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
          VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image_data,
      static_cast<uint32_t>(width * height * sizeof(float)),
      img_staging_buffer);

  VkImageSubresourceRange img_range{
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .baseMipLevel = 0,
      .levelCount = new_image->mip_levels,
      .baseArrayLayer = 0,
      .layerCount = VK_REMAINING_ARRAY_LAYERS,
  };

  vkEndCommandBuffer(copy_cmd_buf);

  VkSubmitInfo submitInfo{
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .commandBufferCount = 1,
      .pCommandBuffers = &copy_cmd_buf,
  };

  VK_CHECK_RESULT(
      vkQueueSubmit(device_->queues.graphics, 1, &submitInfo, VK_NULL_HANDLE));

  // TODO: REMOVE
  vkDeviceWaitIdle(device_->GetLogicalDevice());

  blu::core::Image::CreateImageView(device_->GetLogicalDevice(), new_image,
                                    COLOR_FORMAT, img_range);
  blu::core::Image::CreateImageSampler(
      device_->GetLogicalDevice(), device_->GetDeviceProperties(), new_image);

  vkFreeCommandBuffers(device_->GetLogicalDevice(),
                       graphics_command_pools_[frame_index_], 1, &copy_cmd_buf);
  img_staging_buffer->Destroy(allocator_);
  delete img_staging_buffer;

  free(image_data);

  VkDescriptorImageInfo image_info = {
      .sampler = new_image->sampler,
      .imageView = new_image->view,
      .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
  };

  VkWriteDescriptorSet write_descriptor_set = {
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .dstSet = textures_descriptor_set_->set,
      .dstBinding = 0,
      .dstArrayElement = static_cast<uint32_t>(textures.size()),
      .descriptorCount = 1,
      .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
      .pImageInfo = &image_info,
  };

  vkUpdateDescriptorSets(device_->GetLogicalDevice(), 1, &write_descriptor_set,
                         0, nullptr);

  auto index = textures.size();
  loaded_texture_indices_[filepath] = index;
  textures.push_back(new_image);
  return index;
}

void ForwardRenderer::Prepare() {
  auto frame_count = swapchain_->GetImageCount();
  // Command Pool Creation
  {
    transfer_command_pools.resize(frame_count);
    graphics_command_pools_.resize(frame_count);
    compute_command_pools_.resize(frame_count);

    VkCommandPoolCreateInfo command_pool_create{
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
    };

    for (size_t i = 0; i < frame_count; i++) {
      command_pool_create.queueFamilyIndex =
          device_->queue_family_indicies_.transfer;
      vkCreateCommandPool(device_->GetLogicalDevice(), &command_pool_create,
                          nullptr, &transfer_command_pools[i]);
      command_pool_create.queueFamilyIndex =
          device_->queue_family_indicies_.graphics;
      vkCreateCommandPool(device_->GetLogicalDevice(), &command_pool_create,
                          nullptr, &graphics_command_pools_[i]);
      command_pool_create.queueFamilyIndex =
          device_->queue_family_indicies_.compute;
      vkCreateCommandPool(device_->GetLogicalDevice(), &command_pool_create,
                          nullptr, &compute_command_pools_[i]);
    }

    draw_command_buffers.resize(frame_count);
    dcg_buffers.resize(frame_count);

    VkCommandBufferAllocateInfo command_buffer_alloc_info{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = nullptr,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };

    for (size_t i = 0; i < frame_count; i++) {
      /*command_buffer_alloc_info.commandPool = *transfer_command_pools[i];
      vkAllocateCommandBuffers(device_->GetLogicalDevice(),
                               &command_buffer_alloc_info,
                               draw_command_buffers[i]);*/
      command_buffer_alloc_info.commandPool = graphics_command_pools_[i];
      vkAllocateCommandBuffers(device_->GetLogicalDevice(),
                               &command_buffer_alloc_info,
                               &draw_command_buffers[i]);
      command_buffer_alloc_info.commandPool = compute_command_pools_[i];
      vkAllocateCommandBuffers(device_->GetLogicalDevice(),
                               &command_buffer_alloc_info, &dcg_buffers[i]);
    }
  }

  // Descriptor Pool Creation
  {
    // One VK_DESCRIPTOR_TYPE_STORAGE_BUFFER for holding references to Buffer
    // Device Pointers
    eastl::vector<VkDescriptorPoolSize> pool_sizes{
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MAX_TEXTURES},
    };

    VkDescriptorPoolCreateInfo descriptor_pool_create{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .flags = 0,
        .maxSets = 2,
        .poolSizeCount = static_cast<uint32_t>(pool_sizes.size()),
        .pPoolSizes = pool_sizes.data(),
    };

    VK_CHECK_RESULT(vkCreateDescriptorPool(device_->GetLogicalDevice(),
                                           &descriptor_pool_create, nullptr,
                                           &render_descriptor_pool_));
  }

  // Depth Stencil Creation
  {
    depth_stencil_image_ = blu::core::Image::CreateImage(
        device_->GetLogicalDevice(), allocator_, DEPTH_FORMAT,
        swapchain_->GetWidth(), swapchain_->GetHeight(), 1,
        VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    VkImageSubresourceRange depth_range{
        .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
        .baseMipLevel = 0,
        .levelCount = VK_REMAINING_MIP_LEVELS,
        .baseArrayLayer = 0,
        .layerCount = VK_REMAINING_ARRAY_LAYERS,
    };

    blu::core::Image::CreateImageView(device_->GetLogicalDevice(),
                                      depth_stencil_image_, DEPTH_FORMAT,
                                      depth_range);
  }

  // Buffer Infos Buffer & Descriptor Creation
  // I would like to use more than one buffer so that each frame
  // could access frame specific resources
  {
    buffer_infos_buffer_ = blu::core::Buffer::CreateBuffer(
        device_->GetLogicalDevice(), allocator_,
        sizeof(BufferInfo) * MAX_BUFFERS_STORAGE,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
            VK_MEMORY_PROPERTY_HOST_CACHED_BIT,
        VMA_ALLOCATION_CREATE_MAPPED_BIT);

    VkDescriptorSetLayoutBinding buffer_metadata_binding{
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT |
                      VK_SHADER_STAGE_FRAGMENT_BIT,
    };

    VkDescriptorSetLayoutCreateInfo layout_info{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = 1,
        .pBindings = &buffer_metadata_binding,
    };

    buffer_infos_descriptor_set_ = new blu::core::DescriptorSet();
    vkCreateDescriptorSetLayout(device_->GetLogicalDevice(), &layout_info,
                                nullptr, &buffer_infos_descriptor_set_->layout);

    VkDescriptorSetAllocateInfo descriptor_alloc_info{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = render_descriptor_pool_,
        .descriptorSetCount = 1,
        .pSetLayouts = &buffer_infos_descriptor_set_->layout,
    };
    vkAllocateDescriptorSets(device_->GetLogicalDevice(),
                             &descriptor_alloc_info,
                             &buffer_infos_descriptor_set_->set);

    VkDescriptorBufferInfo descriptor_buffer_info{
        .buffer =
            buffer_infos_buffer_->buffer,  // The SSBO holding buffer metadata
        .offset = 0,
        .range = VK_WHOLE_SIZE,
    };

    VkWriteDescriptorSet descriptor_write{
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = buffer_infos_descriptor_set_->set,
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .pBufferInfo = &descriptor_buffer_info,
    };

    vkUpdateDescriptorSets(device_->GetLogicalDevice(), 1, &descriptor_write, 0,
                           nullptr);
  }

  // Model Textures
  {
    VkDescriptorSetLayoutBinding binding = {
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = MAX_TEXTURES,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        .pImmutableSamplers = nullptr,
    };

    eastl::array<VkDescriptorBindingFlags, 1> flags{
        VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT};

    VkDescriptorSetLayoutBindingFlagsCreateInfo layout_info_flags{
        .sType =
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
        .bindingCount = static_cast<uint32_t>(flags.size()),
        .pBindingFlags = flags.data(),
    };

    // Create descriptor set layout
    VkDescriptorSetLayoutCreateInfo layout_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = &layout_info_flags,
        .bindingCount = 1,
        .pBindings = &binding,
    };
    textures_descriptor_set_ = new blu::core::DescriptorSet();
    vkCreateDescriptorSetLayout(device_->GetLogicalDevice(), &layout_info,
                                nullptr, &textures_descriptor_set_->layout);

    VkDescriptorSetAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = render_descriptor_pool_,
        .descriptorSetCount = 1,
        .pSetLayouts = &textures_descriptor_set_->layout,
    };

    vkAllocateDescriptorSets(device_->GetLogicalDevice(), &alloc_info,
                             &textures_descriptor_set_->set);
  }

  // Buffer Creation
  {
    matrices_buffer_ = blu::core::Buffer::CreateBuffer(
        device_->GetLogicalDevice(), allocator_,
        sizeof(glm::mat4) * (3 + MAX_MODELS),
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
            VK_MEMORY_PROPERTY_HOST_CACHED_BIT,
        VMA_ALLOCATION_CREATE_MAPPED_BIT);

    buffer_infos_.push_back(BufferInfo(matrices_buffer_->device_address,
                                       matrices_buffer_->offset,
                                       matrices_buffer_->size));

    memcpy(buffer_infos_buffer_->mapped_data, buffer_infos_.data(),
           buffer_infos_.size() * sizeof(BufferInfo));

    vertex_buffer_ = blu::core::Buffer::CreateBuffer(
        device_->GetLogicalDevice(), allocator_,
        sizeof(Vertex::pos) * MAX_VERTICES,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    normal_buffer_ = blu::core::Buffer::CreateBuffer(
        device_->GetLogicalDevice(), allocator_,
        sizeof(Vertex::norm) * MAX_VERTICES,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    index_buffer_ = blu::core::Buffer::CreateBuffer(
        device_->GetLogicalDevice(), allocator_, sizeof(uint32_t) * MAX_INDICES,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    uv_buffer_ = blu::core::Buffer::CreateBuffer(
        device_->GetLogicalDevice(), allocator_,
        sizeof(Vertex::uv) * MAX_VERTICES,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  }

  // DCG Buffer Creation
  {
    dcg_input_model_data_.resize(frame_count);
    dcg_input_models_.resize(frame_count);
    dcg_output_buffers_.resize(frame_count);

    for (size_t i = 0; i < frame_count; i++) {
      dcg_input_model_data_[i] = blu::core::Buffer::CreateBuffer(
          device_->GetLogicalDevice(), allocator_,
          sizeof(ModelIndices) * MAX_MODELS,
          VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
              VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
              VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
              VK_MEMORY_PROPERTY_HOST_CACHED_BIT,
          VMA_ALLOCATION_CREATE_MAPPED_BIT);

      dcg_input_models_[i] = blu::core::Buffer::CreateBuffer(
          device_->GetLogicalDevice(), allocator_,
          sizeof(uint32_t) * MAX_MODELS,
          VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
              VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
              VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
              VK_MEMORY_PROPERTY_HOST_CACHED_BIT,
          VMA_ALLOCATION_CREATE_MAPPED_BIT);

      dcg_output_buffers_[i] = blu::core::Buffer::CreateBuffer(
          device_->GetLogicalDevice(), allocator_,
          DRAW_COMMAND_BUFFER_SIZE * MAX_MODELS,
          VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
              VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT |
              VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    }
  }

  // Pipeline Creation
  {
    blu::core::rendering::ComputePipelineCreateInfo dcg_compute_create_info{
        //.descriptor_set_layouts = {dcg_descriptor_set_->layout},
        .shader = LoadShader("shaders/dcg_opaque.comp.spv",
                             VK_SHADER_STAGE_COMPUTE_BIT),
        .push_const = {VkPushConstantRange(VK_SHADER_STAGE_COMPUTE_BIT, 0,
                                           sizeof(DCGPushConst))},
    };

    dcg_pipeline_ =
        new blu::core::rendering::Pipeline(device_, dcg_compute_create_info);

    blu::core::rendering::GraphicsPipelineCreateInfo
        triangle_pipeline_create_info{
            .descriptor_set_layouts = {buffer_infos_descriptor_set_->layout},
            .input_assembly_flags = 0,
            .input_assembly_topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            .input_assembly_primitive_restart_enable = VK_FALSE,
            .rasteriazation_flags = 0,
            .rasteriazation_state_polygone_mode = VK_POLYGON_MODE_FILL,
            .rasteriazation_state_cull_mode = VK_CULL_MODE_FRONT_BIT,
            .rasteriazation_state_front_face = VK_FRONT_FACE_COUNTER_CLOCKWISE,
            .color_blend_attachment_states = {{
                .blendEnable = VK_FALSE,
                .colorWriteMask = 0xf /*RGBA*/,
            }},
            .depth_stencil_depth_test = VK_FALSE,
            .depth_stencil_depth_write = VK_FALSE,
            .depth_stencil_depth_compare_op = VK_COMPARE_OP_LESS,
            .depth_stencil_front_compare_op = VK_COMPARE_OP_ALWAYS,
            .depth_stencil_back_compare_op = VK_COMPARE_OP_ALWAYS,
            .viewport_count = 1,
            .scissor_count = 1,
            .multisample_flags = 0,
            .multisample_count = VK_SAMPLE_COUNT_1_BIT,
            .dynamic_state_flags = 0,
            .dynamic_state_enables = {VK_DYNAMIC_STATE_VIEWPORT,
                                      VK_DYNAMIC_STATE_SCISSOR},
            //.vertex_input_bindings =
            //    {
            //        {0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX},
            //    },
            //.vertex_input_attributes =
            //    {
            //        {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos)},
            //    },
            .color_attachment_formats = {swapchain_->GetColorFormat()},
            .depth_format = DEPTH_FORMAT,

            .shaders{
                LoadShader("shaders/postProcessing.vert.spv",
                           VK_SHADER_STAGE_VERTEX_BIT),
                LoadShader("shaders/postProcessing.frag.spv",
                           VK_SHADER_STAGE_FRAGMENT_BIT),
            },
        };

    triangle_pipeline_ = new blu::core::rendering::Pipeline(
        device_, triangle_pipeline_create_info);

    blu::core::rendering::GraphicsPipelineCreateInfo cube_pipeline_create_info{
        .descriptor_set_layouts =
            {
                buffer_infos_descriptor_set_->layout,
                textures_descriptor_set_->layout,
            },
        .input_assembly_flags = 0,
        .input_assembly_topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .input_assembly_primitive_restart_enable = VK_FALSE,
        .rasteriazation_flags = 0,
        .rasteriazation_state_polygone_mode = VK_POLYGON_MODE_FILL,
        .rasteriazation_state_cull_mode = VK_CULL_MODE_BACK_BIT,
        .rasteriazation_state_front_face = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .color_blend_attachment_states = {{
            .blendEnable = VK_FALSE,
            .colorWriteMask = 0xf /*RGBA*/,
        }},
        .depth_stencil_depth_test = VK_TRUE,
        .depth_stencil_depth_write = VK_TRUE,
        .depth_stencil_depth_compare_op = VK_COMPARE_OP_LESS,
        .depth_stencil_front_compare_op = VK_COMPARE_OP_ALWAYS,
        .depth_stencil_back_compare_op = VK_COMPARE_OP_ALWAYS,
        .viewport_count = 1,
        .scissor_count = 1,
        .multisample_flags = 0,
        .multisample_count = VK_SAMPLE_COUNT_1_BIT,
        .dynamic_state_flags = 0,
        .dynamic_state_enables = {VK_DYNAMIC_STATE_VIEWPORT,
                                  VK_DYNAMIC_STATE_SCISSOR},
        .vertex_input_bindings =
            {
                {0, 3 * sizeof(float), VK_VERTEX_INPUT_RATE_VERTEX},  // POS
                {1, 3 * sizeof(float), VK_VERTEX_INPUT_RATE_VERTEX},  // NORM
                {2, 3 * sizeof(float), VK_VERTEX_INPUT_RATE_VERTEX},  // UV
            },
        .vertex_input_attributes =
            {
                {0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0},  // POS
                {1, 1, VK_FORMAT_R32G32B32_SFLOAT, 0},  // NORM
                {2, 2, VK_FORMAT_R32G32B32_SFLOAT, 0},  // UV
            },
        .color_attachment_formats = {swapchain_->GetColorFormat()},
        .depth_format = DEPTH_FORMAT,
        .shaders{
            LoadShader("shaders/cube.vert.spv", VK_SHADER_STAGE_VERTEX_BIT),
            LoadShader("shaders/cube.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT),
        },
    };

    cube_pipeline_ =
        new blu::core::rendering::Pipeline(device_, cube_pipeline_create_info);
  }

  // Create Fence & Semaphores
  {
    image_available_semaphores_.resize(frame_count);
    render_finished_semaphores_.resize(frame_count);
    dcg_semaphores_.resize(frame_count);
    in_flight_fences_.resize(frame_count);

    VkSemaphoreCreateInfo semaphore_info{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };

    VkFenceCreateInfo fence_info{
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };
    for (size_t i = 0; i < frame_count; i++) {
      vkCreateSemaphore(device_->GetLogicalDevice(), &semaphore_info, nullptr,
                        &image_available_semaphores_[i]);
      vkCreateSemaphore(device_->GetLogicalDevice(), &semaphore_info, nullptr,
                        &render_finished_semaphores_[i]);
      vkCreateSemaphore(device_->GetLogicalDevice(), &semaphore_info, nullptr,
                        &dcg_semaphores_[i]);
      vkCreateFence(device_->GetLogicalDevice(), &fence_info, nullptr,
                    &in_flight_fences_[i]);
    }
  }
}

// Update Render Data
// -Matrices
// Render
// Setup Fences & Semaphores
RendererState ForwardRenderer::Render(RenderData render_data) {
  vkWaitForFences(device_->GetLogicalDevice(), 1,
                  &in_flight_fences_[frame_index_], VK_TRUE, UINT64_MAX);

  auto swapchain = swapchain_->GetSwapchain();
  // Handle Window Resize
  auto acquire_image_result = vkAcquireNextImageKHR(
      device_->GetLogicalDevice(), swapchain, UINT64_MAX,
      image_available_semaphores_[frame_index_], nullptr, &image_index_);

  if ((acquire_image_result == VK_ERROR_OUT_OF_DATE_KHR) ||
      (acquire_image_result == VK_SUBOPTIMAL_KHR)) {
    OnResize();
    return RendererState::ASPECT_RATIO_UPDATED;
  }

  vkResetFences(device_->GetLogicalDevice(), 1,
                &in_flight_fences_[frame_index_]);

  // DCG_COMMAND_GEN
  {
    // Update Model Buffers
    // Does not need to happen every frame, only on update
    memcpy(dcg_input_model_data_[frame_index_]->mapped_data,
           model_indices_.data(), model_indices_.size() * sizeof(ModelIndices));

    memcpy(dcg_input_models_[frame_index_]->mapped_data,
           render_data.model_ids.data(),
           render_data.model_ids.size() * sizeof(uint32_t));

    VkCommandBuffer dcg_command = dcg_buffers[frame_index_];
    vkResetCommandBuffer(dcg_command, 0);

    VkCommandBufferBeginInfo dcg_cmd_buffer_begin{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = nullptr,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = nullptr,
    };

    vkBeginCommandBuffer(dcg_command, &dcg_cmd_buffer_begin);

    vkCmdBindPipeline(dcg_command, VK_PIPELINE_BIND_POINT_COMPUTE,
                      *dcg_pipeline_->GetPipeline());

    DCGPushConst dcg_push_const{
        .input_model_data = BufferInfo(
            dcg_input_model_data_[frame_index_]->device_address, 0, 0),
        .input_models =
            BufferInfo(dcg_input_models_[frame_index_]->device_address, 0, 0),
        .output_command_data =
            BufferInfo(dcg_output_buffers_[frame_index_]->device_address, 0, 0),
        .draw_count = static_cast<uint32_t>(render_data.model_ids.size()),
    };

    vkCmdPushConstants(dcg_command, *dcg_pipeline_->GetPipelineLayout(),
                       VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(DCGPushConst),
                       &dcg_push_const);

    // NVIDIA warp size is 32, AMD is 64
    //  Heavy parallel work is better on smaller worksizes
    //  Memory heavy accesses can work better on larger workgroup sizes

    uint32_t workgroupSizeX = 32;
    uint32_t workgroupSizeY = 32;
    uint32_t workgroupSizeZ = 1;

    vkCmdDispatch(dcg_command, workgroupSizeX, workgroupSizeY, workgroupSizeZ);

    vkEndCommandBuffer(dcg_command);

    VkSubmitInfo draw_info{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &dcg_command,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &dcg_semaphores_[frame_index_],
    };

    VK_CHECK_RESULT(
        vkQueueSubmit(device_->queues.compute, 1, &draw_info, nullptr));
  }

  // Render
  {
    auto swapchain_buf = swapchain_->GetSwapchainBuffer(image_index_);
    auto width = swapchain_->GetWidth();
    auto height = swapchain_->GetHeight();
    // Graphics Queue
    {
      memcpy(matrices_buffer_->mapped_data, render_data.matrices.data(),
             render_data.matrices.size() * sizeof(glm::mat4));

      VkCommandBuffer draw_cmd_buffer = draw_command_buffers[frame_index_];
      vkResetCommandBuffer(draw_cmd_buffer, 0);

      VkCommandBufferBeginInfo draw_cmd_buffer_begin{
          .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
          .pNext = nullptr,
          .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
          .pInheritanceInfo = nullptr,
      };

      vkBeginCommandBuffer(draw_cmd_buffer, &draw_cmd_buffer_begin);

      VkImageSubresourceRange range{
          .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
          .baseMipLevel = 0,
          .levelCount = VK_REMAINING_MIP_LEVELS,
          .baseArrayLayer = 0,
          .layerCount = VK_REMAINING_ARRAY_LAYERS,
      };

      VkImageSubresourceRange depth_range{
          .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
          .baseMipLevel = 0,
          .levelCount = 1,
          .baseArrayLayer = 0,
          .layerCount = 1,
      };

      // Clears & prepares Image memory
      blu::core::Image::ImageLayoutTransition(
          draw_cmd_buffer, swapchain_buf.image, VK_IMAGE_LAYOUT_UNDEFINED,
          VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, range);

      // Clears & prepares Image memory
      blu::core::Image::ImageLayoutTransition(
          draw_cmd_buffer, depth_stencil_image_->image,
          VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
          depth_range);

      eastl::array<VkClearValue, 2> clear_values{};
      clear_values[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
      clear_values[1].depthStencil = {1.0f, 0};

      VkRenderingAttachmentInfo color_attachment_info{
          .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
          .imageView = swapchain_buf.view,
          .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
          .resolveMode = VK_RESOLVE_MODE_NONE,
          .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
          .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
          .clearValue = clear_values[0],
      };

      VkRenderingAttachmentInfo depth_attachment_info{
          .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
          .imageView = depth_stencil_image_->view,
          .imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
          .resolveMode = VK_RESOLVE_MODE_NONE,
          .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
          .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
          .clearValue = clear_values[1],
      };
      VkRenderingInfo render_info{
          .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
          .renderArea = {{.x = 0, .y = 0}, width, height},
          .layerCount = 1,
          .viewMask = 0,
          .colorAttachmentCount = 1,
          .pColorAttachments = &color_attachment_info,
          .pDepthAttachment = &depth_attachment_info,
          //.pStencilAttachment = &depth_attachment_info,
      };

      vkCmdBeginRendering(draw_cmd_buffer, &render_info);

      VkViewport viewport{
          .width = static_cast<float>(width),
          .height = static_cast<float>(height),
          .minDepth = 0.0f,
          .maxDepth = 1.0f,
      };

      vkCmdSetViewport(draw_cmd_buffer, 0, 1, &viewport);

      VkRect2D scissor{
          .offset{.x = 0, .y = 0},
          .extent{
              .width = width,
              .height = height,
          },
      };

      vkCmdSetScissor(draw_cmd_buffer, 0, 1, &scissor);
      VkDeviceSize offsets[1] = {0};

      vkCmdBindPipeline(draw_cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                        *cube_pipeline_->GetPipeline());
      eastl::array<VkDescriptorSet, 2> descriptor_sets{
          buffer_infos_descriptor_set_->set,
          textures_descriptor_set_->set,
      };

      vkCmdBindDescriptorSets(draw_cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                              *cube_pipeline_->GetPipelineLayout(), 0,
                              descriptor_sets.size(), descriptor_sets.data(), 0,
                              nullptr);

      vkCmdBindVertexBuffers(draw_cmd_buffer, 0, 1, &vertex_buffer_->buffer,
                             offsets);
      vkCmdBindVertexBuffers(draw_cmd_buffer, 1, 1, &normal_buffer_->buffer,
                             offsets);
      vkCmdBindVertexBuffers(draw_cmd_buffer, 2, 1, &uv_buffer_->buffer,
                             offsets);

      vkCmdBindIndexBuffer(draw_cmd_buffer, index_buffer_->buffer, 0,
                           VK_INDEX_TYPE_UINT32);

      vkCmdDrawIndexedIndirect(
          draw_cmd_buffer, dcg_output_buffers_[frame_index_]->buffer, 0,
          render_data.model_ids.size(), DRAW_COMMAND_BUFFER_SIZE);

      vkCmdEndRendering(draw_cmd_buffer);

      blu::core::Image::ImageLayoutTransition(
          draw_cmd_buffer, swapchain_buf.image,
          VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
          VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, range);

      vkEndCommandBuffer(draw_cmd_buffer);

      VkSemaphore draw_wait_semaphores[] = {
          image_available_semaphores_[frame_index_],
          dcg_semaphores_[frame_index_]};

      VkPipelineStageFlags draw_wait_stages[] = {
          VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
          VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT,
      };

      VkSemaphore draw_signal_semaphores[] = {
          render_finished_semaphores_[frame_index_]};

      VkSubmitInfo draw_info{
          .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
          .waitSemaphoreCount = 2,
          .pWaitSemaphores = draw_wait_semaphores,
          .pWaitDstStageMask = draw_wait_stages,
          .commandBufferCount = 1,
          .pCommandBuffers = &draw_cmd_buffer,
          .signalSemaphoreCount = 1,
          .pSignalSemaphores = draw_signal_semaphores,
      };

      VK_CHECK_RESULT(vkQueueSubmit(device_->queues.graphics, 1, &draw_info,
                                    in_flight_fences_[frame_index_]));

      VkPresentInfoKHR present_info = {
          .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
          .pNext = NULL,
          .waitSemaphoreCount = 1,
          .pWaitSemaphores = draw_signal_semaphores,
          .swapchainCount = 1,
          .pSwapchains = &swapchain,
          .pImageIndices = &image_index_,
      };

      auto queue_present_result =
          vkQueuePresentKHR(device_->queues.graphics, &present_info);

      if ((queue_present_result == VK_ERROR_OUT_OF_DATE_KHR) ||
          (queue_present_result == VK_SUBOPTIMAL_KHR)) {
        OnResize();
        return RendererState::ASPECT_RATIO_UPDATED;
      }
    }
  }

  frame_index_ = (frame_index_ + 1) % swapchain_->GetImageCount();

  return RendererState::OK;
}

void ForwardRenderer::OnResize() {
  depth_stencil_image_->Destroy(device_->GetLogicalDevice(), allocator_);
  delete depth_stencil_image_;

  swapchain_->Create(false, false);

  depth_stencil_image_ = blu::core::Image::CreateImage(
      device_->GetLogicalDevice(), allocator_, DEPTH_FORMAT,
      swapchain_->GetWidth(), swapchain_->GetHeight(), 1, VK_SAMPLE_COUNT_1_BIT,
      VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  VkImageSubresourceRange depth_range{
      .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
      .baseMipLevel = 0,
      .levelCount = VK_REMAINING_MIP_LEVELS,
      .baseArrayLayer = 0,
      .layerCount = VK_REMAINING_ARRAY_LAYERS,
  };

  blu::core::Image::CreateImageView(device_->GetLogicalDevice(),
                                    depth_stencil_image_, DEPTH_FORMAT,
                                    depth_range);
}

VkPipelineShaderStageCreateInfo ForwardRenderer::LoadShader(
    eastl::string file_name, VkShaderStageFlagBits stage) {
  VkPipelineShaderStageCreateInfo shader_stage{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .stage = stage,
      .module = blu::core::file::LoadShader(file_name.c_str(),
                                            device_->GetLogicalDevice()),
      .pName = "main",
  };

  assert(shader_stage.module != VK_NULL_HANDLE);
  shader_modules_.push_back(shader_stage.module);

  return shader_stage;
}

void* __cdecl operator new[](size_t size, const char* name, int flags,
                             unsigned debugFlags, const char* file, int line) {
  return new uint8_t[size];
}