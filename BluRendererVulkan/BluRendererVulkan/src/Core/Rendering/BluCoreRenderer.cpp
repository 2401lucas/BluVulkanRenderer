#include "BluCoreRenderer.h"

#include <EASTL/array.h>

#include "../External/FileManager.h"
#include "../External/stb_image.h"

BluCoreRenderer::BluCoreRenderer(blu::core::Window* window) {
  window_ = window;

  // VkInstance Creation
  {
    eastl::vector<eastl::string> instance_extensions = {
        VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
#ifdef DEBUG_LABELS
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#endif
    };

    instance_ =
        new blu::core::Instance("Forward Renderer", instance_extensions);

#ifdef DEBUG_LABELS
    debug_util_.vkCmdBeginDebugUtilsLabelEXT =
        (PFN_vkCmdBeginDebugUtilsLabelEXT)vkGetInstanceProcAddr(
            instance_->Get(), "vkCmdBeginDebugUtilsLabelEXT");

    debug_util_.vkCmdEndDebugUtilsLabelEXT =
        (PFN_vkCmdEndDebugUtilsLabelEXT)vkGetInstanceProcAddr(
            instance_->Get(), "vkCmdEndDebugUtilsLabelEXT");

    debug_util_.vkCmdInsertDebugUtilsLabelEXT =
        (PFN_vkCmdInsertDebugUtilsLabelEXT)vkGetInstanceProcAddr(
            instance_->Get(), "vkCmdInsertDebugUtilsLabelEXT");

    debug_util_.vkSetDebugUtilsObjectNameEXT =
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

  Build();
}

BluCoreRenderer::~BluCoreRenderer() {
  vkDeviceWaitIdle(device_->GetLogicalDevice());

  delete vertex_buffer_;
  delete index_buffer_;
  delete normal_buffer_;
  delete uv_buffer_;

  for (auto stage : loaded_stages) {
    delete stage;
  }
  for (auto it = shader_modules_.begin(), it_end = shader_modules_.end();
       it != it_end; ++it) {
    vkDestroyShaderModule(device_->GetLogicalDevice(), it->second, nullptr);
  }
  for (auto it = image_samplers_.begin(), it_end = image_samplers_.end();
       it != it_end; ++it) {
    vkDestroySampler(device_->GetLogicalDevice(), it->second, nullptr);
  }
  for (auto& semaphore : semaphores_) {
    vkDestroySemaphore(device_->GetLogicalDevice(), semaphore, nullptr);
  }
  for (auto& fence : fences_) {
    vkDestroyFence(device_->GetLogicalDevice(), fence, nullptr);
  }
  for (auto& pool : command_pools_) {
    vkDestroyCommandPool(device_->GetLogicalDevice(), pool, nullptr);
  }
  for (auto& pool : descriptor_pools_) {
    vkDestroyDescriptorPool(device_->GetLogicalDevice(), pool, nullptr);
  }
  for (auto& layout : descriptor_set_layouts) {
    vkDestroyDescriptorSetLayout(device_->GetLogicalDevice(), layout, nullptr);
  }

  vmaDestroyAllocator(allocator_);
  delete swapchain_;
  delete device_;
  delete instance_;
}

eastl::vector<BluCoreRenderer::LoadedModelInfo> BluCoreRenderer::LoadModel(
    eastl::string file) {
  eastl::vector<BluCoreRenderer::LoadedModelInfo> output;

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
        .commandPool = command_pools_[0],
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

      GPUModelIndices model_index_data{
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
          LoadedModelInfo(model_indices_.size(), mesh->GetBoundingSphere()));
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

    vkFreeCommandBuffers(device_->GetLogicalDevice(), command_pools_[0], 1,
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

  return output;
}

int BluCoreRenderer::LoadImage(eastl::string filepath) {
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
      .commandPool = command_pools_[1],
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
      device_->GetLogicalDevice(), new_image,
      device_->GetDeviceProperties().limits.maxSamplerAnisotropy);

  vkFreeCommandBuffers(device_->GetLogicalDevice(), command_pools_[1], 1,
                       &copy_cmd_buf);
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
      .dstSet = textures_descriptor_set_,
      .dstBinding = 0,
      .dstArrayElement = static_cast<uint32_t>(model_textures_.size()),
      .descriptorCount = 1,
      .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
      .pImageInfo = &image_info,
  };

  vkUpdateDescriptorSets(device_->GetLogicalDevice(), 1, &write_descriptor_set,
                         0, nullptr);

  auto index = model_textures_.size();
  loaded_texture_indices_[filepath] = index;
  model_textures_.push_back(new_image);
  return index;
}

void BluCoreRenderer::LoadTexture(
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

void BluCoreRenderer::Build() {
  VkCommandPoolCreateInfo command_pool_ci{
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
      .queueFamilyIndex = device_->queue_family_indicies_.transfer,
  };

  // Create Transfer Command Pool for Async Transfers
  CreateCommandPool(command_pool_ci);
  // Create Graphics Pool for image uploads
  command_pool_ci.queueFamilyIndex = device_->queue_family_indicies_.graphics;
  CreateCommandPool(command_pool_ci);
  // Create Descriptor Pool for Global Resources(BDA, Textures)
  eastl::vector<VkDescriptorPoolSize> pool_sizes{
      {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1},
      {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MAX_TEXTURES},
  };

  VkDescriptorPoolCreateInfo descriptor_pool_create{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT_EXT,
      .maxSets = 2,
      .poolSizeCount = static_cast<uint32_t>(pool_sizes.size()),
      .pPoolSizes = pool_sizes.data(),
  };

  CreateDescriptorPool(descriptor_pool_create);

  // BDA Buffer & Descriptor Set Creation
  {
    bda_buffer_ = blu::core::Buffer::CreateBuffer(
        device_->GetLogicalDevice(), allocator_,
        sizeof(BufferInfo) * MAX_BUFFERS_STORAGE,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
            VK_MEMORY_PROPERTY_HOST_CACHED_BIT,
        VMA_ALLOCATION_CREATE_MAPPED_BIT);

#if DEBUG_LABELS
    VkDebugUtilsObjectNameInfoEXT debug_info{
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
        .objectType = VK_OBJECT_TYPE_BUFFER,
        .objectHandle = (uint64_t)bda_buffer_->buffer,
        .pObjectName = "BDA Buffer",
    };

    debug_util_.vkSetDebugUtilsObjectNameEXT(device_->GetLogicalDevice(),
                                             &debug_info);
#endif

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

    vkCreateDescriptorSetLayout(device_->GetLogicalDevice(), &layout_info,
                                nullptr, &bda_buffer_descriptor_set_layout_);

    VkDescriptorSetAllocateInfo descriptor_alloc_info{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = descriptor_pools_[0],
        .descriptorSetCount = 1,
        .pSetLayouts = &bda_buffer_descriptor_set_layout_,
    };
    vkAllocateDescriptorSets(device_->GetLogicalDevice(),
                             &descriptor_alloc_info,
                             &bda_buffer_descriptor_set_);

#if DEBUG_LABELS
    debug_info = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
        .objectType = VK_OBJECT_TYPE_DESCRIPTOR_SET,
        .objectHandle = (uint64_t)bda_buffer_descriptor_set_,
        .pObjectName = "BDA DescriptorSet",
    };

    debug_util_.vkSetDebugUtilsObjectNameEXT(device_->GetLogicalDevice(),
                                             &debug_info);
#endif

    VkDescriptorBufferInfo descriptor_buffer_info{
        .buffer = bda_buffer_->buffer,  // The SSBO holding buffer metadata
        .offset = 0,
        .range = VK_WHOLE_SIZE,
    };

    VkWriteDescriptorSet descriptor_write{
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = bda_buffer_descriptor_set_,
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .pBufferInfo = &descriptor_buffer_info,
    };

    vkUpdateDescriptorSets(device_->GetLogicalDevice(), 1, &descriptor_write, 0,
                           nullptr);
  }

  // Textures Descriptor Set
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
    vkCreateDescriptorSetLayout(device_->GetLogicalDevice(), &layout_info,
                                nullptr, &textures_descriptor_set_layout_);

    VkDescriptorSetAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = descriptor_pools_[0],
        .descriptorSetCount = 1,
        .pSetLayouts = &textures_descriptor_set_layout_,
    };
    vkAllocateDescriptorSets(device_->GetLogicalDevice(), &alloc_info,
                             &textures_descriptor_set_);

#if DEBUG_LABELS
    VkDebugUtilsObjectNameInfoEXT debug_info = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
        .objectType = VK_OBJECT_TYPE_DESCRIPTOR_SET,
        .objectHandle = (uint64_t)textures_descriptor_set_,
        .pObjectName = "Textures DescriptorSet",
    };

    debug_util_.vkSetDebugUtilsObjectNameEXT(device_->GetLogicalDevice(),
                                             &debug_info);
#endif
  }

  // Model Buffers
  {
    vertex_buffer_ = blu::core::Buffer::CreateBuffer(
        device_->GetLogicalDevice(), allocator_,
        sizeof(glm::vec3) * MAX_VERTICES,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
#if DEBUG_LABELS
    VkDebugUtilsObjectNameInfoEXT debug_info = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
        .objectType = VK_OBJECT_TYPE_BUFFER,
        .objectHandle = (uint64_t)vertex_buffer_->buffer,
        .pObjectName = "Vertex Buffer",
    };

    debug_util_.vkSetDebugUtilsObjectNameEXT(device_->GetLogicalDevice(),
                                             &debug_info);
#endif
    normal_buffer_ = blu::core::Buffer::CreateBuffer(
        device_->GetLogicalDevice(), allocator_,
        sizeof(glm::vec3) * MAX_VERTICES,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
#if DEBUG_LABELS
    debug_info = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
        .objectType = VK_OBJECT_TYPE_BUFFER,
        .objectHandle = (uint64_t)normal_buffer_->buffer,
        .pObjectName = "Normal Buffer",
    };

    debug_util_.vkSetDebugUtilsObjectNameEXT(device_->GetLogicalDevice(),
                                             &debug_info);
#endif
    index_buffer_ = blu::core::Buffer::CreateBuffer(
        device_->GetLogicalDevice(), allocator_, sizeof(uint32_t) * MAX_INDICES,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
#if DEBUG_LABELS
    debug_info = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
        .objectType = VK_OBJECT_TYPE_BUFFER,
        .objectHandle = (uint64_t)index_buffer_->buffer,
        .pObjectName = "Index Buffer",
    };

    debug_util_.vkSetDebugUtilsObjectNameEXT(device_->GetLogicalDevice(),
                                             &debug_info);
#endif
    uv_buffer_ = blu::core::Buffer::CreateBuffer(
        device_->GetLogicalDevice(), allocator_,
        sizeof(glm::vec3) * MAX_VERTICES,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
#if DEBUG_LABELS
    debug_info = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
        .objectType = VK_OBJECT_TYPE_BUFFER,
        .objectHandle = (uint64_t)uv_buffer_->buffer,
        .pObjectName = "UV Buffer",
    };

    debug_util_.vkSetDebugUtilsObjectNameEXT(device_->GetLogicalDevice(),
                                             &debug_info);
#endif
  }
}

uint64_t BluCoreRenderer::GetNextSemaphoreValue() {
  current_semaphore_value_ += 1;
  return current_semaphore_value_;
}

void BluCoreRenderer::StartCommandBuffer(VkCommandBuffer buf, const char* name,
                                         glm::vec4 rgb) {
  vkResetCommandBuffer(buf, 0);

  VkCommandBufferBeginInfo cmd_buf_begin{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .pNext = nullptr,
      .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
      .pInheritanceInfo = nullptr,
  };

  vkBeginCommandBuffer(buf, &cmd_buf_begin);

#ifdef DEBUG_LABELS
  VkDebugUtilsLabelEXT label_info{
      .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
      .pNext = nullptr,
      .pLabelName = name,
      .color = {rgb.x, rgb.y, rgb.z, rgb.w}};

  debug_util_.vkCmdBeginDebugUtilsLabelEXT(buf, &label_info);
#endif
}

void BluCoreRenderer::EndCommandBuffer(VkCommandBuffer buf) {
#ifdef DEBUG_LABELS
  debug_util_.vkCmdEndDebugUtilsLabelEXT(buf);
#endif
  vkEndCommandBuffer(buf);
}

void BluCoreRenderer::SubmitCommandBuffer(
    eastl::vector<VkCommandBuffer> cmd_bufs, VkQueue& queue,
    eastl::vector<VkSemaphore> wait_semaphores,
    eastl::vector<uint64_t> wait_values,
    eastl::vector<VkPipelineStageFlags> wait_flags,
    eastl::vector<VkSemaphore> signal_semaphores,
    eastl::vector<uint64_t> signal_values, VkFence fence) {
  VkTimelineSemaphoreSubmitInfo timeline_info{
      .sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO,
      .waitSemaphoreValueCount = static_cast<uint32_t>(wait_values.size()),
      .pWaitSemaphoreValues = wait_values.data(),
      .signalSemaphoreValueCount = static_cast<uint32_t>(signal_values.size()),
      .pSignalSemaphoreValues = signal_values.data(),
  };

  VkSubmitInfo submit_info{
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .pNext = &timeline_info,
      .waitSemaphoreCount = static_cast<uint32_t>(wait_semaphores.size()),
      .pWaitSemaphores = wait_semaphores.data(),
      .pWaitDstStageMask = wait_flags.data(),
      .commandBufferCount = static_cast<uint32_t>(cmd_bufs.size()),
      .pCommandBuffers = cmd_bufs.data(),
      .signalSemaphoreCount = static_cast<uint32_t>(signal_semaphores.size()),
      .pSignalSemaphores = signal_semaphores.data(),
  };

  VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submit_info, fence));
}

blu::core::Image* BluCoreRenderer::CreateRenderTargetImage(
    VkFormat format, uint32_t width, uint32_t height, uint32_t mip_levels,
    VkSampleCountFlagBits samples, VkImageTiling tiling,
    VkImageUsageFlags usage, VkMemoryPropertyFlags required_flags,
    VmaAllocationCreateFlags flags) {
  uint32_t id = render_images_.size();
  render_images_.push_back(blu::core::Image::CreateImage(
      device_->GetLogicalDevice(), allocator_, format, width, height,
      mip_levels, samples, tiling, usage, required_flags, flags));
  return render_images_[id];
}

blu::core::Buffer* BluCoreRenderer::CreateRenderTargetBuffer(
    VkDeviceSize size, VkBufferUsageFlags usage,
    VkMemoryPropertyFlags required_flags, VmaAllocationCreateFlags flags) {
  uint32_t id = render_buffers_.size();
  render_buffers_.push_back(
      blu::core::Buffer::CreateBuffer(device_->GetLogicalDevice(), allocator_,
                                      size, usage, required_flags, flags));
  return render_buffers_[id];
}

VkSemaphore& BluCoreRenderer::CreateSemaphore(
    VkSemaphoreCreateInfo& create_info) {
  uint32_t id = semaphores_.size();
  semaphores_.push_back();
  vkCreateSemaphore(device_->GetLogicalDevice(), &create_info, nullptr,
                    &semaphores_[id]);
  return semaphores_[id];
}

VkFence& BluCoreRenderer::CreateFence(VkFenceCreateInfo& create_info) {
  uint32_t id = fences_.size();
  fences_.push_back();
  vkCreateFence(device_->GetLogicalDevice(), &create_info, nullptr,
                &fences_[id]);
  return fences_[id];
}

VkCommandPool& BluCoreRenderer::CreateCommandPool(
    VkCommandPoolCreateInfo& create_info) {
  uint32_t id = command_pools_.size();
  command_pools_.push_back();
  vkCreateCommandPool(device_->GetLogicalDevice(), &create_info, nullptr,
                      &command_pools_[id]);
  return command_pools_[id];
}

VkDescriptorPool& BluCoreRenderer::CreateDescriptorPool(
    VkDescriptorPoolCreateInfo& create_info) {
  uint32_t id = descriptor_pools_.size();
  descriptor_pools_.push_back();
  vkCreateDescriptorPool(device_->GetLogicalDevice(), &create_info, nullptr,
                         &descriptor_pools_[id]);
  return descriptor_pools_[id];
}

VkDescriptorSetLayout& BluCoreRenderer::CreateDescriptorSetLayout(
    VkDescriptorSetLayoutCreateInfo& create_info) {
  uint32_t id = descriptor_set_layouts.size();
  descriptor_set_layouts.push_back();
  vkCreateDescriptorSetLayout(device_->GetLogicalDevice(), &create_info,
                              nullptr, &descriptor_set_layouts[id]);
  return descriptor_set_layouts[id];
}

VkPipelineShaderStageCreateInfo BluCoreRenderer::LoadShader(
    eastl::string file_name, VkShaderStageFlagBits stage) {
  VkShaderModule shader_module = VK_NULL_HANDLE;

  if (shader_modules_.find(file_name) != shader_modules_.end()) {
    shader_module = shader_modules_[file_name];
  } else {
    shader_module = blu::core::file::LoadShader(file_name.c_str(),
                                                device_->GetLogicalDevice());
    shader_modules_[file_name] = shader_module;
  }

  assert(shader_module != VK_NULL_HANDLE);

  VkPipelineShaderStageCreateInfo shader_stage{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .stage = stage,
      .module = shader_module,
      .pName = "main",
  };

#if DEBUG_LABELS
  VkDebugUtilsObjectNameInfoEXT debug_info{
      .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
      .objectType = VK_OBJECT_TYPE_SHADER_MODULE,
      .objectHandle = (uint64_t)shader_stage.module,
      .pObjectName = file_name.c_str(),
  };

  debug_util_.vkSetDebugUtilsObjectNameEXT(device_->GetLogicalDevice(),
                                           &debug_info);
#endif

  return shader_stage;
}

void* __cdecl operator new[](size_t size, const char* name, int flags,
                             unsigned debugFlags, const char* file, int line) {
  return new uint8_t[size];
}