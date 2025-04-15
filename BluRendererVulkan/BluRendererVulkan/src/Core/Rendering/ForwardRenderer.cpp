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
#ifdef _DEBUG
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#endif
    };

    instance_ = new blu::core::Instance("Forward Renderer", USE_VALIDATION,
                                        instance_extensions);

#ifdef _DEBUG
    debug_util.vkCmdBeginDebugUtilsLabelEXT =
        (PFN_vkCmdBeginDebugUtilsLabelEXT)vkGetInstanceProcAddr(
            instance_->Get(), "vkCmdBeginDebugUtilsLabelEXT");

    debug_util.vkCmdEndDebugUtilsLabelEXT =
        (PFN_vkCmdEndDebugUtilsLabelEXT)vkGetInstanceProcAddr(
            instance_->Get(), "vkCmdEndDebugUtilsLabelEXT");

    debug_util.vkCmdInsertDebugUtilsLabelEXT =
        (PFN_vkCmdInsertDebugUtilsLabelEXT)vkGetInstanceProcAddr(
            instance_->Get(), "vkCmdInsertDebugUtilsLabelEXT");

    debug_util.vkSetDebugUtilsObjectNameEXT =
        (PFN_vkSetDebugUtilsObjectNameEXT)vkGetInstanceProcAddr(
            instance_->Get(), "vkSetDebugUtilsObjectNameEXT");
#endif
  }

  // VkDevice Creation
  {
    VkPhysicalDeviceFeatures2 physical_device_features2{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
    };

    VkPhysicalDeviceFeatures physical_device_requested_features_{
        .multiDrawIndirect = VK_TRUE,
        .wideLines = VK_TRUE,
        .samplerAnisotropy = VK_TRUE,
        .shaderInt64 = VK_TRUE,
    };

    physical_device_features2.features = physical_device_requested_features_;
    eastl::vector<const char*> device_extensions = {
        VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
        VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
        VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
        VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
    };

    VkPhysicalDeviceVulkan12Features vulkan_12_features{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
        .drawIndirectCount = VK_TRUE,
        .descriptorIndexing = VK_TRUE,
        .descriptorBindingPartiallyBound = VK_TRUE,
        .runtimeDescriptorArray = VK_TRUE,
        .timelineSemaphore = VK_TRUE,
        .bufferDeviceAddress = VK_TRUE,
    };

    physical_device_features2.pNext = &vulkan_12_features;
    VkPhysicalDeviceDynamicRenderingFeaturesKHR dynamic_rendering{
        .sType =
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR,
        .dynamicRendering = VK_TRUE,
    };
    vulkan_12_features.pNext = &dynamic_rendering;

    VkPhysicalDeviceSynchronization2FeaturesKHR device_sync{
        .sType =
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES_KHR,
        .synchronization2 = VK_TRUE,
    };
    dynamic_rendering.pNext = &device_sync;

    device_ = new blu::core::Device(instance_, physical_device_features2,
                                    device_extensions);
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
    vkDestroySemaphore(device_->GetLogicalDevice(), present_semaphores[i],
                       nullptr);
    vkDestroySemaphore(device_->GetLogicalDevice(),
                       image_available_semaphore[i], nullptr);
    vkDestroyFence(device_->GetLogicalDevice(), in_flight_fences_[i], nullptr);
  }

  vkDestroySemaphore(device_->GetLogicalDevice(), frame_semaphore, nullptr);
  vkDestroyDescriptorPool(device_->GetLogicalDevice(), render_descriptor_pool_,
                          nullptr);

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
  models_data_buffer_->Destroy(allocator_);
  delete models_data_buffer_;
  models_buffer_->Destroy(allocator_);
  delete models_buffer_;

  delete build_command_buffer_stage_;
  delete frustum_cull_stage_;
  delete depth_only_stage_;
  delete opaque_render_stage_;
  delete image_copy_stage_;
  delete imgui_stage_;

  if (anti_aliasing_stage_ != nullptr) {
    delete anti_aliasing_stage_;
  }

  for (eastl::vector<blu::core::rendering::ModelData>::iterator
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

  vmaDestroyAllocator(allocator_);
  delete swapchain_;
  delete device_;
  delete instance_;
}

// For now- NOT ASYNC
eastl::vector<ModelInfo> ForwardRenderer::LoadModel(eastl::string file) {
  eastl::vector<ModelInfo> output;
  /* if (loaded_model_indices_.find(file) != loaded_model_indices_.end()) {
    output.push_back(loaded_model_indices_[file]);
    return output;
  }*/

  eastl::string filepath = "assets/" + file;

  auto new_model = blu::core::rendering::ModelData(filepath);
  auto& materials = new_model.GetMaterials();
  auto& meshes = new_model.GetMeshes();
  // Material Data
  {
    size_t pos = filepath.rfind('/');
    eastl::string folderpath;
    if (pos != std::string::npos) {
      folderpath = filepath.substr(0, pos + 1);
    }
    for (auto& mat : materials) {
      LoadTexture(mat.GetBaseColorTextureInfo(), folderpath);
      LoadTexture(mat.GetNormalTextureInfo(), folderpath);
      LoadTexture(mat.GetEmissionTextureInfo(), folderpath);
      LoadTexture(mat.GetMetalnessTextureInfo(), folderpath);
      LoadTexture(mat.GetDiffuseRoughnessTextureInfo(), folderpath);
      LoadTexture(mat.GetAmbientOcclusionTextureInfo(), folderpath);
    }
  }
  // Mesh Data
  {
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

    uint32_t vertex_data_count = 0;
    uint32_t normal_data_count = 0;
    uint32_t index_data_count = 0;
    uint32_t uv_data_count = 0;

    uint32_t vertex_data_offset = 0;
    uint32_t normal_data_offset = 0;
    uint32_t index_data_offset = 0;
    uint32_t uv_data_offset = 0;

    uint32_t vertex_data_total_count = 0;
    uint32_t normal_data_total_count = 0;
    uint32_t index_data_total_count = 0;
    uint32_t uv_data_total_count = 0;

    for (auto& mesh : meshes) {
      vertex_data_total_count += mesh->GetVertexData().count;
      normal_data_total_count += mesh->GetNormalData().count;
      index_data_total_count += mesh->GetIndexData().count;
      uv_data_total_count += mesh->GetUVData().count;
    }

    char* vertex_data = new char[sizeof(float) * 3 * vertex_data_total_count];
    char* normal_data = new char[sizeof(float) * 3 * normal_data_total_count];
    char* index_data = new char[sizeof(uint32_t) * index_data_total_count];
    char* uv_data = new char[sizeof(float) * 3 * uv_data_total_count];

    for (auto& mesh : meshes) {
      auto& material = materials[mesh->GetMaterialIndex()];

      auto& mesh_vertex_data = mesh->GetVertexData();
      auto& mesh_normal_data = mesh->GetNormalData();
      auto& mesh_index_data = mesh->GetIndexData();
      auto& mesh_uv_data = mesh->GetUVData();

      ModelIndices model_index_data{
          .vert_offset =
              static_cast<int>(vertex_buffer_data_count_ + vertex_data_count),
          .ind_count = mesh_index_data.count,
          .ind_offset = index_buffer_data_count_ + index_data_count,
          .base_tex_id = material.GetBaseColorTextureInfo().index,
          .normal_tex_id = material.GetNormalTextureInfo().index,
          .emission_tex_id = material.GetEmissionTextureInfo().index,
          .metalness_tex_id = material.GetMetalnessTextureInfo().index,
          .diffuse_roughness_id =
              material.GetDiffuseRoughnessTextureInfo().index,
          .ambient_occlusion_id =
              material.GetAmbientOcclusionTextureInfo().index,
      };
      output.push_back(
          ModelInfo(model_indices_.size(), mesh->GetBoundingSphere()));
      model_indices_.push_back(model_index_data);

      memcpy(vertex_data + vertex_data_offset, mesh_vertex_data.data,
             mesh_vertex_data.data_size);
      memcpy(normal_data + normal_data_offset, mesh_normal_data.data,
             mesh_normal_data.data_size);
      memcpy(index_data + index_data_offset, mesh_index_data.data,
             mesh_index_data.data_size);
      memcpy(uv_data + uv_data_offset, mesh_uv_data.data,
             mesh_uv_data.data_size);

      vertex_data_offset += mesh_vertex_data.data_size;
      vertex_data_count += mesh_vertex_data.count;
      normal_data_offset += mesh_normal_data.data_size;
      index_data_offset += mesh_index_data.data_size;
      index_data_count += mesh_index_data.count;
      uv_data_offset += mesh_uv_data.data_size;
    }

    blu::core::Buffer* vertex_staging_buffer;
    blu::core::Buffer::UploadToBuffer(
        device_->GetLogicalDevice(), allocator_, vertex_buffer_,
        sizeof(float) * 3 * vertex_buffer_data_count_, copy_cmd_buf,
        vertex_data, sizeof(float) * 3 * vertex_data_total_count, 0,
        vertex_staging_buffer);
    vertex_buffer_data_count_ += vertex_data_total_count;

    blu::core::Buffer* normal_staging_buffer;
    blu::core::Buffer::UploadToBuffer(
        device_->GetLogicalDevice(), allocator_, normal_buffer_,
        sizeof(float) * 3 * normal_buffer_data_count_, copy_cmd_buf,
        normal_data, sizeof(float) * 3 * normal_data_total_count, 0,
        normal_staging_buffer);
    normal_buffer_data_count_ += normal_data_total_count;

    blu::core::Buffer* index_staging_buffer;
    blu::core::Buffer::UploadToBuffer(
        device_->GetLogicalDevice(), allocator_, index_buffer_,
        sizeof(uint32_t) * index_buffer_data_count_, copy_cmd_buf, index_data,
        sizeof(uint32_t) * index_data_total_count, 0, index_staging_buffer);
    index_buffer_data_count_ += index_data_total_count;

    blu::core::Buffer* uv_staging_buffer;
    blu::core::Buffer::UploadToBuffer(
        device_->GetLogicalDevice(), allocator_, uv_buffer_,
        sizeof(float) * 3 * uv_buffer_data_count_, copy_cmd_buf, uv_data,
        sizeof(float) * 3 * uv_data_total_count, 0, uv_staging_buffer);
    uv_buffer_data_count_ += uv_data_total_count;

    vkEndCommandBuffer(copy_cmd_buf);

    VkSubmitInfo submitInfo{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &copy_cmd_buf,
    };

    vkQueueSubmit(device_->queues.transfer, 1, &submitInfo, VK_NULL_HANDLE);
    // TODO: REMOVE
    vkDeviceWaitIdle(device_->GetLogicalDevice());

    vkFreeCommandBuffers(device_->GetLogicalDevice(),
                         transfer_command_pools[frame_index_], 1,
                         &copy_cmd_buf);

    vertex_staging_buffer->Destroy(allocator_);
    delete vertex_staging_buffer;
    delete[] vertex_data;

    normal_staging_buffer->Destroy(allocator_);
    delete normal_staging_buffer;
    delete[] normal_data;

    index_staging_buffer->Destroy(allocator_);
    delete index_staging_buffer;
    delete[] index_data;

    uv_staging_buffer->Destroy(allocator_);
    delete uv_staging_buffer;
    delete[] uv_data;
  }

  models_data_buffer_updated = true;

  return output;
}

int ForwardRenderer::LoadImage(eastl::string filepath) {
  if (loaded_texture_indices_.find(filepath) != loaded_texture_indices_.end()) {
    return loaded_texture_indices_[filepath];
  }

  int width, height, channels;
  unsigned char* image_data =
      stbi_load(filepath.c_str(), &width, &height, &channels, STBI_rgb_alpha);
  if (!image_data) {
    std::cerr << "Failed to load image at " << filepath.c_str() << std::endl;
    return 0;
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

void ForwardRenderer::LoadTexture(
    blu::core::rendering::Material::TextureInfo& info,
    eastl::string folderpath) {
  if (!info.filepath.empty()) {
#ifdef DEBUG_UV
    info.index = 0;
#else
    info.index = LoadImage(folderpath + info.filepath);
#endif
  }
}

void ForwardRenderer::GenerateResources() {
  auto frame_count = swapchain_->GetImageCount();
  auto width = swapchain_->GetWidth();
  auto height = swapchain_->GetHeight();
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

    // This is horrific and should be illigal to do...
    // but it works. I WILL fix this at somepoint soon
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
    models_data_buffer_ = blu::core::Buffer::CreateBuffer(
        device_->GetLogicalDevice(), allocator_,
        sizeof(ModelIndices) * MAX_MODELS,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
            VK_MEMORY_PROPERTY_HOST_CACHED_BIT,
        VMA_ALLOCATION_CREATE_MAPPED_BIT);

    models_buffer_ = blu::core::Buffer::CreateBuffer(
        device_->GetLogicalDevice(), allocator_,
        sizeof(glm::vec4) * 6 + sizeof(ModelData) * MAX_MODELS,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
            VK_MEMORY_PROPERTY_HOST_CACHED_BIT,
        VMA_ALLOCATION_CREATE_MAPPED_BIT);

    buffer_infos_.push_back(BufferInfo(models_data_buffer_->device_address,
                                       models_data_buffer_->offset,
                                       models_data_buffer_->size));
    buffer_infos_.push_back(BufferInfo(models_buffer_->device_address,
                                       models_buffer_->offset,
                                       models_buffer_->size));
    memcpy(buffer_infos_buffer_->mapped_data, buffer_infos_.data(),
           buffer_infos_.size() * sizeof(BufferInfo));
  }

  // Stage Creation
  {
    // build_command_buffer_stage_
    {
      blu::core::rendering::ComputePipelineCreateInfo
          build_command_buffer_create_info{
              .shader = LoadShader("shaders/build_command_buffer.comp.spv",
                                   VK_SHADER_STAGE_COMPUTE_BIT),
              .push_const = {VkPushConstantRange(
                  VK_SHADER_STAGE_COMPUTE_BIT, 0,
                  sizeof(blu::core::rendering::BuildCommandBufferStage::
                             BuildCommandBufferPushConst))},
          };

      auto build_command_buffer_pipeline = new blu::core::rendering::Pipeline(
          device_, build_command_buffer_create_info);

      build_command_buffer_stage_ =
          new blu::core::rendering::BuildCommandBufferStage(
              device_, allocator_, build_command_buffer_pipeline,
              compute_command_pools_);
    }
    // frustum_cull_stage_
    {
      blu::core::rendering::ComputePipelineCreateInfo frustum_cull_create_info{
          .shader = LoadShader("shaders/frustum_cull.comp.spv",
                               VK_SHADER_STAGE_COMPUTE_BIT),
          .push_const = {VkPushConstantRange(
              VK_SHADER_STAGE_COMPUTE_BIT, 0,
              sizeof(
                  blu::core::rendering::FrustumCullStage::FrustumPushConst))},
      };

      auto frustum_cull_pipeline =
          new blu::core::rendering::Pipeline(device_, frustum_cull_create_info);

      frustum_cull_stage_ = new blu::core::rendering::FrustumCullStage(
          device_, allocator_, frustum_cull_pipeline, compute_command_pools_);
    }

    // depth_only_stage_
    {
      blu::core::rendering::GraphicsPipelineCreateInfo
          depth_only_stage_create_info{
              .descriptor_set_layouts = {buffer_infos_descriptor_set_->layout},
              .input_assembly_flags = 0,
              .input_assembly_topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
              .input_assembly_primitive_restart_enable = VK_FALSE,
              .rasteriazation_flags = 0,
              .rasteriazation_state_polygone_mode = VK_POLYGON_MODE_FILL,
              .rasteriazation_state_cull_mode = VK_CULL_MODE_FRONT_BIT,
              .rasteriazation_state_front_face =
                  VK_FRONT_FACE_COUNTER_CLOCKWISE,
              .color_blend_attachment_states = {},
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
                      {0, 3 * sizeof(float),
                       VK_VERTEX_INPUT_RATE_VERTEX},  // POS
                  },
              .vertex_input_attributes =
                  {
                      {0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0},  // POS
                  },
              .color_attachment_formats = {},
              .depth_format = DEPTH_FORMAT,

              .shaders{LoadShader("shaders/depth_only.vert.spv",
                                  VK_SHADER_STAGE_VERTEX_BIT)},
          };

      auto depth_only_stage_pipeline = new blu::core::rendering::Pipeline(
          device_, depth_only_stage_create_info);

      depth_only_stage_ = new blu::core::rendering::DepthOnlyStage(
          device_, allocator_, depth_only_stage_pipeline,
          graphics_command_pools_, width, height);
    }

    // opaque_render_stage_
    {
      blu::core::rendering::GraphicsPipelineCreateInfo
          opaque_render_stage_create_info{
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
              .rasteriazation_state_front_face =
                  VK_FRONT_FACE_COUNTER_CLOCKWISE,
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
                      {0, 3 * sizeof(float),
                       VK_VERTEX_INPUT_RATE_VERTEX},  // POS
                      {1, 3 * sizeof(float),
                       VK_VERTEX_INPUT_RATE_VERTEX},  // NORM
                      {2, 3 * sizeof(float),
                       VK_VERTEX_INPUT_RATE_VERTEX},  // UV
                  },
              .vertex_input_attributes =
                  {
                      {0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0},  // POS
                      {1, 1, VK_FORMAT_R32G32B32_SFLOAT, 0},  // NORM
                      {2, 2, VK_FORMAT_R32G32B32_SFLOAT, 0},  // UV
                  },
              .color_attachment_formats = {COLOR_FORMAT},
              .depth_format = DEPTH_FORMAT,
              .shaders{
                  LoadShader("shaders/cube.vert.spv",
                             VK_SHADER_STAGE_VERTEX_BIT),
                  LoadShader("shaders/cube.frag.spv",
                             VK_SHADER_STAGE_FRAGMENT_BIT),
              },
          };

      auto opaque_render_stage_pipeline_ = new blu::core::rendering::Pipeline(
          device_, opaque_render_stage_create_info);

      opaque_render_stage_ = new blu::core::rendering::OpaqueRenderStage(
          device_, allocator_, opaque_render_stage_pipeline_,
          graphics_command_pools_, width, height);
    }

    // image_copy_stage_
    {
      image_copy_stage_ = new blu::core::rendering::ImageCopyStage(
          device_, allocator_, graphics_command_pools_);
    }

    // anti_aliasing_stage_
    {
      // blu::core::rendering::ComputePipelineCreateInfo
      // anti_aliasing_create_info{
      //     //.descriptor_set_layouts = // TODO
      //     .shader = LoadShader("shaders/anti_aliasing.comp.spv",
      //                          VK_SHADER_STAGE_COMPUTE_BIT),
      // };

      // auto anti_aliasing_pipeline = new blu::core::rendering::Pipeline(
      //     device_, anti_aliasing_create_info);

      // anti_aliasing_stage_ = new blu::core::rendering::AntiAliasingStage(
      //     device_, allocator_, anti_aliasing_pipeline,
      //     compute_command_pools_, width, height);
    }

    // imgui_ui_stage
    {
      imgui_stage_ = new blu::core::rendering::ImGuiStage(
          instance_, device_, window_, frame_count, graphics_command_pools_);
    }
  }

  // Create Fence & Semaphores
  {
    present_semaphores.resize(frame_count);
    image_available_semaphore.resize(frame_count);
    in_flight_fences_.resize(frame_count);

    VkSemaphoreCreateInfo semaphore_info{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};

    VkFenceCreateInfo fence_info{
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };
    for (size_t i = 0; i < frame_count; i++) {
      vkCreateSemaphore(device_->GetLogicalDevice(), &semaphore_info, nullptr,
                        &present_semaphores[i]);
      vkCreateSemaphore(device_->GetLogicalDevice(), &semaphore_info, nullptr,
                        &image_available_semaphore[i]);
      vkCreateFence(device_->GetLogicalDevice(), &fence_info, nullptr,
                    &in_flight_fences_[i]);
    }
    VkSemaphoreTypeCreateInfoKHR type_create_info{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO_KHR,
        .semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE_KHR,
        .initialValue = 0,
    };
    semaphore_info.pNext = &type_create_info;

    vkCreateSemaphore(device_->GetLogicalDevice(), &semaphore_info, nullptr,
                      &frame_semaphore);

    LoadImage("assets/uv-test.png");  // Default Tex
  }
}

bool ForwardRenderer::PrepareFrame() {
  vkWaitForFences(device_->GetLogicalDevice(), 1,
                  &in_flight_fences_[frame_index_], VK_TRUE, UINT64_MAX);

  auto acquire_image_result = vkAcquireNextImageKHR(
      device_->GetLogicalDevice(), swapchain_->GetSwapchain(), UINT64_MAX,
      image_available_semaphore[frame_index_], nullptr, &image_index_);

  if ((acquire_image_result == VK_ERROR_OUT_OF_DATE_KHR) ||
      (acquire_image_result == VK_SUBOPTIMAL_KHR)) {
    return false;
  }

  vkResetFences(device_->GetLogicalDevice(), 1,
                &in_flight_fences_[frame_index_]);

  return true;
}

void ForwardRenderer::UpdateFrameData(RenderData& render_data) {
  // FRAME DATA UPDATING
  if (models_data_buffer_updated) {
    memcpy(models_data_buffer_->mapped_data, model_indices_.data(),
           model_indices_.size() * sizeof(ModelIndices));
    models_data_buffer_updated = false;
  }
  memcpy(models_buffer_->mapped_data, render_data.scene.planes,
         sizeof(glm::vec4) * 6);
  memcpy(models_buffer_->mapped_data + sizeof(glm::vec4) * 6,
         render_data.scene.model_data.data(),
         sizeof(ModelData) * render_data.scene.model_data.size());
  models_buffer_updated = false;
  vmaFlushAllocation(allocator_, models_buffer_->alloc, 0, VK_WHOLE_SIZE);
  memcpy(matrices_buffer_->mapped_data, render_data.matrices.data(),
         render_data.matrices.size() * sizeof(glm::mat4));
}

void ForwardRenderer::BuildFrameTimeline() {
  // Resets Timeline Semaphore values
  semaphore_values = {};

  switch (settings_.culling_mode) {
    case CULLING_MODE_NONE:
      semaphore_values.build_command_buffer_stage_ = GetNextSemaphoreValue();
      break;
    case CULLING_MODE_FRUSTUM_CULL:
      semaphore_values.frustum_cull_stage_ = GetNextSemaphoreValue();
      break;
    case CULLING_MODE_OCCLUSION_CULL:
      semaphore_values.frustum_cull_stage_ = GetNextSemaphoreValue();
      semaphore_values.depth_only_stage_ = GetNextSemaphoreValue();
      semaphore_values.occlusion_cull_stage_ = GetNextSemaphoreValue();
      break;
    default:
      break;
  }
  semaphore_values.cull_mode_complete = current_semaphore_value;

  switch (settings_.draw_mode) {
    case DRAW_MODE_SHADED:
      semaphore_values.opaque_render_stage_ = GetNextSemaphoreValue();
      break;
    case DRAW_MODE_UNLIT:
      semaphore_values.unlit_opaque_render_stage_ = GetNextSemaphoreValue();
      break;
    case DRAW_MODE_WIREFRAME:
      semaphore_values.wireframe_render_stage_ = GetNextSemaphoreValue();
      break;
    default:
      break;
  }
  semaphore_values.draw_mode_complete = current_semaphore_value;

  switch (settings_.aliasing) {
    case ANTI_ALIAS_MODE_NONE:
      break;
    case ANTI_ALIAS_MODE_FXAA:
      semaphore_values.anti_aliasing_stage_ = GetNextSemaphoreValue();
      break;
  }
  semaphore_values.anti_aliasing_mode_complete = current_semaphore_value;
}

bool ForwardRenderer::PresentFrame(blu::core::Image* target_image,
                                   uint64_t wait_semaphore_value) {
  auto swapchain = swapchain_->GetSwapchain();
  auto swapchain_buf = swapchain_->GetSwapchainBuffer(frame_index_);

  image_copy_stage_->Run(
      frame_index_, target_image->image, target_image->layout,
      swapchain_buf.image, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
      swapchain_->GetWidth(), swapchain_->GetHeight(), frame_semaphore,
      wait_semaphore_value, VK_PIPELINE_STAGE_TRANSFER_BIT,
      present_semaphores[frame_index_], UINT64_MAX,
      in_flight_fences_[frame_index_]);

  VkSemaphore present_wait_semaphore[] = {
      present_semaphores[frame_index_],
      image_available_semaphore[frame_index_]};

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

RendererState ForwardRenderer::Render(RenderData render_data) {
  // TODO:
  //  UPDATE INFO SOONER (IE while prev frame is in last stages, allow cur frame
  //  first stages to run)
  // Based on output of BuildFrameTimeline, Each stage should be updated
  // accordingly
  //  Something similar to->
  // If StageSignalValue is 0, skip
  // Else Record Commands & run
  // Render Settings Should not control individual stages but complete
  // operations: An example would be Occlusion Culling, which requires the
  // Frustum & Depth only stages
  // Settings should be, No Cull, Frustum Cull, Occl. Cull.
  // Recording would then be
  // NoCullStage
  // FrustumCullStage
  // FrustumCullStage -> DepthOnlyStage->OcclusionCullStage

  BuildFrameTimeline();

  // Swapchain Acquire Next image
  if (!PrepareFrame()) {
    OnResize();
    return RendererState::ASPECT_RATIO_UPDATED;
  }

  UpdateFrameData(render_data);

  blu::core::Buffer* draw_command_buffer;

  switch (settings_.culling_mode) {
    case CULLING_MODE_NONE:
      build_command_buffer_stage_->Run(
          frame_index_, BufferInfo(models_data_buffer_->device_address),
          BufferInfo(models_buffer_->device_address),
          render_data.scene.model_data.size(), nullptr, UINT64_MAX,
          VK_PIPELINE_STAGE_NONE, frame_semaphore,
          semaphore_values.build_command_buffer_stage_, nullptr);
      draw_command_buffer = build_command_buffer_stage_
                                ->command_buffer_output_buffers_[frame_index_];
      break;
    case CULLING_MODE_FRUSTUM_CULL:
      frustum_cull_stage_->Run(
          frame_index_, BufferInfo(models_data_buffer_->device_address),
          BufferInfo(models_buffer_->device_address),
          render_data.scene.model_data.size(), nullptr, UINT64_MAX,
          VK_PIPELINE_STAGE_NONE, frame_semaphore,
          semaphore_values.frustum_cull_stage_, nullptr);
      draw_command_buffer =
          frustum_cull_stage_->frustum_output_buffers_[frame_index_];
      break;
    case CULLING_MODE_OCCLUSION_CULL:
      frustum_cull_stage_->Run(
          frame_index_, BufferInfo(models_data_buffer_->device_address),
          BufferInfo(models_buffer_->device_address),
          render_data.scene.model_data.size(), nullptr, UINT64_MAX,
          VK_PIPELINE_STAGE_NONE, frame_semaphore,
          semaphore_values.frustum_cull_stage_, nullptr);
      depth_only_stage_->Run(
          frame_index_,
          frustum_cull_stage_->frustum_output_buffers_[frame_index_],
          {buffer_infos_descriptor_set_->set}, vertex_buffer_, index_buffer_,
          frame_semaphore, semaphore_values.frustum_cull_stage_,
          VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT, frame_semaphore,
          semaphore_values.depth_only_stage_, nullptr);
      // OCCL.STAGE
      break;
  }

  switch (settings_.draw_mode) {
    case DRAW_MODE_SHADED:
      opaque_render_stage_->Run(
          frame_index_, draw_command_buffer,
          {buffer_infos_descriptor_set_->set, textures_descriptor_set_->set},
          vertex_buffer_, normal_buffer_, uv_buffer_, index_buffer_,
          frame_semaphore, semaphore_values.cull_mode_complete,
          VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT, frame_semaphore,
          semaphore_values.opaque_render_stage_, nullptr);
      break;
    case DRAW_MODE_UNLIT:
      break;
    case DRAW_MODE_WIREFRAME:
      break;
    default:
      break;
  }

  switch (settings_.aliasing) {
    case ANTI_ALIAS_MODE_NONE:
      break;
    case ANTI_ALIAS_MODE_FXAA:
      // anti_aliasing_stage_->Run(
      //     frame_index_, frame_semaphore,
      //     semaphore_values.opaque_render_stage_,
      //     VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, frame_semaphore,
      //     semaphore_values.anti_aliasing_stage_, nullptr);
      break;
  }

  blu::core::Image* output_image;
  switch (settings_.output) {
    case RENDER_OUTPUT_DRAW_STAGE:
      output_image = opaque_render_stage_->color_output_images[frame_index_];
      break;
    case RENDER_OUTPUT_AA:
      output_image = anti_aliasing_stage_->aliased_output_images[frame_index_];
      break;
  }

  if (!PresentFrame(output_image, semaphore_values.draw_mode_complete)) {
    OnResize();
    return RendererState::ASPECT_RATIO_UPDATED;
  }

  frame_index_ = (frame_index_ + 1) % swapchain_->GetImageCount();

  return RendererState::OK;
}

uint64_t ForwardRenderer::GetNextSemaphoreValue() {
  current_semaphore_value += 1;
  return current_semaphore_value;
}

void ForwardRenderer::OnResize() {
  vkDeviceWaitIdle(device_->GetLogicalDevice());

  swapchain_->Create(false, false);

  auto frame_count = swapchain_->GetImageCount();
  auto width = swapchain_->GetWidth();
  auto height = swapchain_->GetHeight();

  if (depth_only_stage_ != nullptr) {
    depth_only_stage_->Resize(frame_count, width, height);
  }
  if (opaque_render_stage_ != nullptr) {
    opaque_render_stage_->Resize(frame_count, width, height);
  }
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