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
#ifdef DEBUG_LABELS
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#endif
    };

    instance_ = new blu::core::Instance("Forward Renderer", USE_VALIDATION,
                                        instance_extensions);

#ifdef DEBUG_LABELS
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
    vkDestroySemaphore(device_->GetLogicalDevice(), ui_semaphores[i], nullptr);
    vkDestroySemaphore(device_->GetLogicalDevice(),
                       image_available_semaphore[i], nullptr);
    vkDestroyFence(device_->GetLogicalDevice(), in_flight_fences_[i], nullptr);
  }

  vkDestroySemaphore(device_->GetLogicalDevice(), main_frame_semaphore,
                     nullptr);
  vkDestroyDescriptorPool(device_->GetLogicalDevice(), render_descriptor_pool_,
                          nullptr);

  buffer_infos_buffer_->Destroy(allocator_);
  delete buffer_infos_buffer_;
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

  buffer_infos_descriptor_set_->Destroy(device_->GetLogicalDevice());
  delete buffer_infos_descriptor_set_;
  textures_descriptor_set_->Destroy(device_->GetLogicalDevice());
  delete textures_descriptor_set_;
  render_images_descriptor_->Destroy(device_->GetLogicalDevice());
  delete render_images_descriptor_;

  delete build_command_buffer_stage_;
  delete frustum_cull_stage_;
  delete depth_only_stage_;
  delete opaque_render_stage_;
  delete image_copy_stage_;
  delete imgui_stage_;
  delete final_composition;
  delete anti_aliasing_stage_;

  for (auto& buf : buffers_draw_command_) {
    buf->Destroy(allocator_);
    delete buf;
  }

  for (auto& img : ui_img_output) {
    img->Destroy(device_->GetLogicalDevice(), allocator_);
    delete img;
  }

  for (auto& img : images_render_assist_color) {
    img->Destroy(device_->GetLogicalDevice(), allocator_);
    delete img;
  }

  for (auto& img : images_render_assist_depth) {
    img->Destroy(device_->GetLogicalDevice(), allocator_);
    delete img;
  }

  for (eastl::vector<blu::core::rendering::ModelData>::iterator
           it = loaded_models_.begin(),
           it_end = loaded_models_.end();
       it != it_end; ++it) {
    it->Delete();
  }

  for (auto it = shader_modules_.begin(), it_end = shader_modules_.end();
       it != it_end; ++it) {
    vkDestroyShaderModule(device_->GetLogicalDevice(), it->second, nullptr);
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
      device_->GetLogicalDevice(), new_image,
      device_->GetDeviceProperties().limits.maxSamplerAnisotropy);

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
      .dstSet = textures_descriptor_set_->sets[0],
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
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
         MAX_TEXTURES + RENDER_ASSIST_IMAGES_PER_FRAME * frame_count},
    };

    VkDescriptorPoolCreateInfo descriptor_pool_create{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT_EXT,
        .maxSets = 2 + frame_count,
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

#if DEBUG_LABELS
    VkDebugUtilsObjectNameInfoEXT debug_info{
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
        .objectType = VK_OBJECT_TYPE_BUFFER,
        .objectHandle = (uint64_t)buffer_infos_buffer_->buffer,
        .pObjectName = "BDA Buffer",
    };

    debug_util.vkSetDebugUtilsObjectNameEXT(device_->GetLogicalDevice(),
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

    buffer_infos_descriptor_set_ = new blu::core::DescriptorSet();
    vkCreateDescriptorSetLayout(device_->GetLogicalDevice(), &layout_info,
                                nullptr, &buffer_infos_descriptor_set_->layout);

    VkDescriptorSetAllocateInfo descriptor_alloc_info{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = render_descriptor_pool_,
        .descriptorSetCount = 1,
        .pSetLayouts = &buffer_infos_descriptor_set_->layout,
    };
    buffer_infos_descriptor_set_->sets.resize(1);
    vkAllocateDescriptorSets(device_->GetLogicalDevice(),
                             &descriptor_alloc_info,
                             &buffer_infos_descriptor_set_->sets[0]);

#if DEBUG_LABELS
    debug_info = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
        .objectType = VK_OBJECT_TYPE_DESCRIPTOR_SET,
        .objectHandle = (uint64_t)buffer_infos_descriptor_set_->sets[0],
        .pObjectName = "BDA DescriptorSet",
    };

    debug_util.vkSetDebugUtilsObjectNameEXT(device_->GetLogicalDevice(),
                                            &debug_info);
#endif

    VkDescriptorBufferInfo descriptor_buffer_info{
        .buffer =
            buffer_infos_buffer_->buffer,  // The SSBO holding buffer metadata
        .offset = 0,
        .range = VK_WHOLE_SIZE,
    };

    VkWriteDescriptorSet descriptor_write{
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = buffer_infos_descriptor_set_->sets[0],
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
    textures_descriptor_set_->sets.resize(1);
    vkAllocateDescriptorSets(device_->GetLogicalDevice(), &alloc_info,
                             &textures_descriptor_set_->sets[0]);
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
#if DEBUG_LABELS
    VkDebugUtilsObjectNameInfoEXT debug_info{
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
        .objectType = VK_OBJECT_TYPE_BUFFER,
        .objectHandle = (uint64_t)matrices_buffer_->buffer,
        .pObjectName = "BDA Buffer",
    };

    debug_util.vkSetDebugUtilsObjectNameEXT(device_->GetLogicalDevice(),
                                            &debug_info);
#endif
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
#if DEBUG_LABELS
    debug_info = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
        .objectType = VK_OBJECT_TYPE_BUFFER,
        .objectHandle = (uint64_t)vertex_buffer_->buffer,
        .pObjectName = "Vertex Buffer",
    };

    debug_util.vkSetDebugUtilsObjectNameEXT(device_->GetLogicalDevice(),
                                            &debug_info);
#endif
    normal_buffer_ = blu::core::Buffer::CreateBuffer(
        device_->GetLogicalDevice(), allocator_,
        sizeof(Vertex::norm) * MAX_VERTICES,
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

    debug_util.vkSetDebugUtilsObjectNameEXT(device_->GetLogicalDevice(),
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

    debug_util.vkSetDebugUtilsObjectNameEXT(device_->GetLogicalDevice(),
                                            &debug_info);
#endif
    uv_buffer_ = blu::core::Buffer::CreateBuffer(
        device_->GetLogicalDevice(), allocator_,
        sizeof(Vertex::uv) * MAX_VERTICES,
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

    debug_util.vkSetDebugUtilsObjectNameEXT(device_->GetLogicalDevice(),
                                            &debug_info);
#endif
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

#if DEBUG_LABELS
    VkDebugUtilsObjectNameInfoEXT debug_info{
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
        .objectType = VK_OBJECT_TYPE_BUFFER,
        .objectHandle = (uint64_t)models_data_buffer_->buffer,
        .pObjectName = "Model Data Buffer",
    };

    debug_util.vkSetDebugUtilsObjectNameEXT(device_->GetLogicalDevice(),
                                            &debug_info);
#endif

    models_buffer_ = blu::core::Buffer::CreateBuffer(
        device_->GetLogicalDevice(), allocator_,
        sizeof(glm::vec4) * 6 + sizeof(ModelData) * MAX_MODELS,
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
        .objectHandle = (uint64_t)models_buffer_->buffer,
        .pObjectName = "Instance Model Info Buffer",
    };

    debug_util.vkSetDebugUtilsObjectNameEXT(device_->GetLogicalDevice(),
                                            &debug_info);
#endif

    buffer_infos_.push_back(BufferInfo(models_data_buffer_->device_address,
                                       models_data_buffer_->offset,
                                       models_data_buffer_->size));
    buffer_infos_.push_back(BufferInfo(models_buffer_->device_address,
                                       models_buffer_->offset,
                                       models_buffer_->size));
    memcpy(buffer_infos_buffer_->mapped_data, buffer_infos_.data(),
           buffer_infos_.size() * sizeof(BufferInfo));
  }

  // Global Descriptor Layouts
  {
    render_images_descriptor_ = new blu::core::DescriptorSet();

    VkDescriptorSetLayoutBinding binding = {
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = RENDER_ASSIST_IMAGES_PER_FRAME,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        .pImmutableSamplers = nullptr,
    };

    VkDescriptorSetLayoutCreateInfo layout_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = 1,
        .pBindings = &binding,
    };
    vkCreateDescriptorSetLayout(device_->GetLogicalDevice(), &layout_info,
                                nullptr, &render_images_descriptor_->layout);
  }

  // Stage Creation
  {
    build_command_buffer_stage_ =
        new blu::core::rendering::stage::BuildCommandBufferStage(
            device_, LoadShader("shaders/build_command_buffer.comp.spv",
                                VK_SHADER_STAGE_COMPUTE_BIT));

    frustum_cull_stage_ = new blu::core::rendering::stage::FrustumCullStage(
        device_, LoadShader("shaders/frustum_cull.comp.spv",
                            VK_SHADER_STAGE_COMPUTE_BIT));

    depth_only_stage_ = new blu::core::rendering::stage::DepthOnlyStage(
        device_, {{buffer_infos_descriptor_set_->layout}},
        LoadShader("shaders/depth_only.vert.spv", VK_SHADER_STAGE_VERTEX_BIT));

    opaque_render_stage_ = new blu::core::rendering::stage::OpaqueRenderStage(
        device_,
        {
            buffer_infos_descriptor_set_->layout,
            textures_descriptor_set_->layout,
        },
        {LoadShader("shaders/cube.vert.spv", VK_SHADER_STAGE_VERTEX_BIT),
         LoadShader("shaders/cube.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT)});

    image_copy_stage_ =
        new blu::core::rendering::stage::ImageCopyStage(device_, allocator_);
    imgui_stage_ = new blu::core::rendering::stage::ImGuiStage(
        instance_, device_, window_, frame_count);

    final_composition = new blu::core::rendering::stage::ColorOnlyStage(
        device_, {render_images_descriptor_->layout},
        {LoadShader("shaders/fullscreen_tri.vert.spv",
                    VK_SHADER_STAGE_VERTEX_BIT),
         LoadShader("shaders/final_composition.frag.spv",
                    VK_SHADER_STAGE_FRAGMENT_BIT)});

    /*anti_aliasing_stage_ = new blu::core::rendering::stage::AntiAliasingStage(
        device_, {anti_aliasing_images_descriptor_->layout},
        LoadShader("shaders/fxaa.comp.spv", VK_SHADER_STAGE_COMPUTE_BIT));*/
  }

  // Create Fence & Semaphores
  {
    present_semaphores.resize(frame_count);
    ui_semaphores.resize(frame_count);
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
                        &ui_semaphores[i]);
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
                      &main_frame_semaphore);

    LoadImage("assets/uv-test.png");  // Default Tex
  }

  OnResize();
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
  // FrustumCullStage->DepthOnlyStage->OcclusionCullStage

  BuildFrameTimeline();

  // Swapchain Acquire Next image
  if (!PrepareFrame()) {
    OnResize();
    return RendererState::ASPECT_RATIO_UPDATED;
  }

  UpdateFrameData(render_data);

  DoDrawUI();
  DoCull(render_data.scene.model_data.size());
  DoDraw();

  if (!DoPresent()) {
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
  VkCommandBufferAllocateInfo command_buffer_alloc_info{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .pNext = nullptr,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = 1,
  };

  if (cmd_bufs_cull_.size() == 0) {
    cmd_bufs_cull_.resize(frame_count);
    for (size_t i = 0; i < frame_count; i++) {
      command_buffer_alloc_info.commandPool = compute_command_pools_[i];
      vkAllocateCommandBuffers(device_->GetLogicalDevice(),
                               &command_buffer_alloc_info, &cmd_bufs_cull_[i]);
    }
  }
  if (cmd_bufs_draw_.size() == 0) {
    cmd_bufs_draw_.resize(frame_count);
    for (size_t i = 0; i < frame_count; i++) {
      command_buffer_alloc_info.commandPool = graphics_command_pools_[i];
      vkAllocateCommandBuffers(device_->GetLogicalDevice(),
                               &command_buffer_alloc_info, &cmd_bufs_draw_[i]);
    }
  }
  if (cmd_bufs_ui_draw_.size() == 0) {
    cmd_bufs_ui_draw_.resize(frame_count);
    for (size_t i = 0; i < frame_count; i++) {
      command_buffer_alloc_info.commandPool = graphics_command_pools_[i];
      vkAllocateCommandBuffers(device_->GetLogicalDevice(),
                               &command_buffer_alloc_info,
                               &cmd_bufs_ui_draw_[i]);
    }
  }
  if (cmd_bufs_present_.size() == 0) {
    cmd_bufs_present_.resize(frame_count);
    for (size_t i = 0; i < frame_count; i++) {
      command_buffer_alloc_info.commandPool = graphics_command_pools_[i];
      vkAllocateCommandBuffers(device_->GetLogicalDevice(),
                               &command_buffer_alloc_info,
                               &cmd_bufs_present_[i]);
    }
  }

  if (buffers_draw_command_.size() != frame_count) {
    for (auto& buf : buffers_draw_command_) {
      buf->Destroy(allocator_);
      delete buf;
    }
    buffers_draw_command_.resize(frame_count);
    for (size_t i = 0; i < frame_count; i++) {
      buffers_draw_command_[i] = blu::core::Buffer::CreateBuffer(
          device_->GetLogicalDevice(), allocator_,
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
          .objectHandle = (uint64_t)buffers_draw_command_[i]->buffer,
          .pObjectName = "Culled Draw Commands",
      };

      debug_util.vkSetDebugUtilsObjectNameEXT(device_->GetLogicalDevice(),
                                              &debug_info);
#endif
    }
  }

  if (ui_img_output.size() != frame_count) {
    for (auto& img : ui_img_output) {
      img->Destroy(device_->GetLogicalDevice(), allocator_);
      delete img;
    }
    ui_img_output.resize(frame_count);
    for (size_t i = 0; i < frame_count; i++) {
      ui_img_output[i] = blu::core::Image::CreateImage(
          device_->GetLogicalDevice(), allocator_, COLOR_FORMAT,
          swapchain_->GetWidth(), swapchain_->GetHeight(), 1,
          VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TILING_OPTIMAL,
          VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
              VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

      VkImageSubresourceRange range{
          .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
          .baseMipLevel = 0,
          .levelCount = VK_REMAINING_MIP_LEVELS,
          .baseArrayLayer = 0,
          .layerCount = VK_REMAINING_ARRAY_LAYERS,
      };

      blu::core::Image::CreateImageView(device_->GetLogicalDevice(),
                                        ui_img_output[i], COLOR_FORMAT, range);
      blu::core::Image::CreateImageSampler(device_->GetLogicalDevice(),
                                           ui_img_output[i], 0);

#if DEBUG_LABELS
      VkDebugUtilsObjectNameInfoEXT debug_info{
          .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
          .objectType = VK_OBJECT_TYPE_IMAGE,
          .objectHandle = (uint64_t)ui_img_output[i]->image,
          .pObjectName = "UI Output Images",
      };

      debug_util.vkSetDebugUtilsObjectNameEXT(device_->GetLogicalDevice(),
                                              &debug_info);
#endif
    }
  }

  if (images_render_assist_color.size() != frame_count) {
    for (auto& img : images_render_assist_color) {
      img->Destroy(device_->GetLogicalDevice(), allocator_);
      delete img;
    }
    images_render_assist_color.resize(frame_count);
    for (size_t i = 0; i < frame_count; i++) {
      images_render_assist_color[i] = blu::core::Image::CreateImage(
          device_->GetLogicalDevice(), allocator_, COLOR_FORMAT,
          swapchain_->GetWidth(), swapchain_->GetHeight(), 1,
          VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TILING_OPTIMAL,
          VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
              VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

      VkImageSubresourceRange range{
          .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
          .baseMipLevel = 0,
          .levelCount = VK_REMAINING_MIP_LEVELS,
          .baseArrayLayer = 0,
          .layerCount = VK_REMAINING_ARRAY_LAYERS,
      };

      blu::core::Image::CreateImageView(device_->GetLogicalDevice(),
                                        images_render_assist_color[i],
                                        COLOR_FORMAT, range);

      blu::core::Image::CreateImageSampler(device_->GetLogicalDevice(),
                                           images_render_assist_color[i], 0);

#if DEBUG_LABELS
      VkDebugUtilsObjectNameInfoEXT debug_info{
          .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
          .objectType = VK_OBJECT_TYPE_IMAGE,
          .objectHandle = (uint64_t)images_render_assist_color[i]->image,
          .pObjectName = "Render Assist Colour Images",
      };

      debug_util.vkSetDebugUtilsObjectNameEXT(device_->GetLogicalDevice(),
                                              &debug_info);
#endif
    }
  }

  if (images_render_assist_depth.size() != frame_count) {
    for (auto& img : images_render_assist_depth) {
      img->Destroy(device_->GetLogicalDevice(), allocator_);
      delete img;
    }
    images_render_assist_depth.resize(frame_count);
    for (size_t i = 0; i < frame_count; i++) {
      images_render_assist_depth[i] = blu::core::Image::CreateImage(
          device_->GetLogicalDevice(), allocator_, DEPTH_FORMAT, width, height,
          1, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TILING_OPTIMAL,
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
                                        images_render_assist_depth[i],
                                        DEPTH_FORMAT, depth_range);
#if DEBUG_LABELS
      VkDebugUtilsObjectNameInfoEXT debug_info{
          .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
          .objectType = VK_OBJECT_TYPE_IMAGE,
          .objectHandle = (uint64_t)images_render_assist_depth[i]->image,
          .pObjectName = "Render Assist Depth Images",
      };

      debug_util.vkSetDebugUtilsObjectNameEXT(device_->GetLogicalDevice(),
                                              &debug_info);
#endif
    }
  }

  {
    render_images_descriptor_->sets.resize(frame_count);
    for (size_t i = 0; i < frame_count; i++) {
      VkDescriptorSetAllocateInfo alloc_info = {
          .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
          .pNext = nullptr,
          .descriptorPool = render_descriptor_pool_,
          .descriptorSetCount = 1,
          .pSetLayouts = &render_images_descriptor_->layout,
      };

      VK_CHECK_RESULT(
          vkAllocateDescriptorSets(device_->GetLogicalDevice(), &alloc_info,
                                   &render_images_descriptor_->sets[i]));

      eastl::vector<VkDescriptorImageInfo> imageInfos;
      imageInfos.resize(RENDER_ASSIST_IMAGES_PER_FRAME);
      imageInfos[0] = {
          .sampler = images_render_assist_color[i]->sampler,
          .imageView = images_render_assist_color[i]->view,
          .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      };
      imageInfos[1] = {
          .sampler = ui_img_output[i]->sampler,
          .imageView = ui_img_output[i]->view,
          .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      };

      VkWriteDescriptorSet write = {
          .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
          .dstSet = render_images_descriptor_->sets[i],
          .dstBinding = 0,
          .descriptorCount = static_cast<uint32_t>(imageInfos.size()),
          .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
          .pImageInfo = imageInfos.data(),
      };

      vkUpdateDescriptorSets(device_->GetLogicalDevice(), 1, &write, 0,
                             nullptr);
    }
  }
}

void ForwardRenderer::DoCull(uint32_t model_count) {
  VkCommandBuffer buf = cmd_bufs_cull_[frame_index_];
  StartCommandBuffer(buf);
#ifdef DEBUG_LABELS
  VkDebugUtilsLabelEXT label_info{
      .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
      .pNext = nullptr,
      .pLabelName = "Cull Models",
      .color = {1, 0, 0, 1}};

  debug_util.vkCmdBeginDebugUtilsLabelEXT(buf, &label_info);
#endif
  switch (settings_.culling_mode) {
    case CULLING_MODE_NONE:
      build_command_buffer_stage_->Run(
          buf, BufferInfo(models_data_buffer_->device_address),
          BufferInfo(models_buffer_->device_address), model_count,
          buffers_draw_command_[frame_index_]);
      break;
    case CULLING_MODE_FRUSTUM_CULL:
      frustum_cull_stage_->Run(
          buf, BufferInfo(models_data_buffer_->device_address),
          BufferInfo(models_buffer_->device_address), model_count,
          buffers_draw_command_[frame_index_]);
      break;
    case CULLING_MODE_OCCLUSION_CULL:
      // frustum_cull_stage_->Run(
      //     frame_index_, BufferInfo(models_data_buffer_->device_address),
      //     BufferInfo(models_buffer_->device_address),
      //     render_data.scene.model_data.size(), nullptr, UINT64_MAX,
      //     VK_PIPELINE_STAGE_NONE, frame_semaphore,
      //     semaphore_values.frustum_cull_stage_, nullptr);
      // depth_only_stage_->Run(
      //     frame_index_,
      //     frustum_cull_stage_->frustum_output_buffers_[frame_index_],
      //     {buffer_infos_descriptor_set_->set}, vertex_buffer_,
      //     index_buffer_, frame_semaphore,
      //     semaphore_values.frustum_cull_stage_,
      //     VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT, frame_semaphore,
      //     semaphore_values.depth_only_stage_, nullptr);
      //  OCCL.STAGE
      break;
  }

#ifdef DEBUG_LABELS
  debug_util.vkCmdEndDebugUtilsLabelEXT(buf);
#endif
  EndCommandBuffer(buf);
  SubmitCommandBuffer({buf}, device_->queues.compute, {}, {}, {},
                      {main_frame_semaphore},
                      {semaphore_values.cull_mode_complete}, nullptr);
}

void ForwardRenderer::DoDraw() {
  VkCommandBuffer buf = cmd_bufs_draw_[frame_index_];
  StartCommandBuffer(buf);

#ifdef DEBUG_LABELS
  VkDebugUtilsLabelEXT label_info{
      .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
      .pNext = nullptr,
      .pLabelName = "Draw Opaque Models",
      .color = {0, 1, 0, 1}};

  debug_util.vkCmdBeginDebugUtilsLabelEXT(buf, &label_info);
#endif
  switch (settings_.draw_mode) {
    case DRAW_MODE_SHADED:
      opaque_render_stage_->Run(
          buf, images_render_assist_color[frame_index_],
          images_render_assist_depth[frame_index_], swapchain_->GetWidth(),
          swapchain_->GetHeight(), buffers_draw_command_[frame_index_],
          {buffer_infos_descriptor_set_->sets[0],
           textures_descriptor_set_->sets[0]},
          vertex_buffer_, normal_buffer_, uv_buffer_, index_buffer_);
      break;
    case DRAW_MODE_UNLIT:
    case DRAW_MODE_WIREFRAME:
    default:
      assert(false && "Not Yet Implemented");
      break;
  }
#ifdef DEBUG_LABELS
  debug_util.vkCmdEndDebugUtilsLabelEXT(buf);
#endif
  EndCommandBuffer(buf);
  SubmitCommandBuffer({buf}, device_->queues.graphics, {main_frame_semaphore},
                      {semaphore_values.cull_mode_complete},
                      {VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT},
                      {main_frame_semaphore},
                      {semaphore_values.draw_mode_complete}, nullptr);
}

void ForwardRenderer::DoAA() {
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
}

void ForwardRenderer::DoDrawUI() {
  auto buf = cmd_bufs_ui_draw_[frame_index_];
  StartCommandBuffer(buf);
#ifdef DEBUG_LABELS
  VkDebugUtilsLabelEXT label_info{
      .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
      .pNext = nullptr,
      .pLabelName = "UI Draw",
      .color = {0, 1, 0, 1}};

  debug_util.vkCmdBeginDebugUtilsLabelEXT(buf, &label_info);
#endif

  imgui_stage_->Start(buf, ui_img_output[frame_index_], swapchain_->GetWidth(),
                      swapchain_->GetHeight());

  ImGui::ShowDebugLogWindow();

  ImGui::ShowDemoWindow();

  imgui_stage_->End(buf);
#ifdef DEBUG_LABELS
  debug_util.vkCmdEndDebugUtilsLabelEXT(buf);
#endif
  EndCommandBuffer(buf);
  SubmitCommandBuffer({buf}, device_->queues.graphics, {}, {}, {},
                      {ui_semaphores[frame_index_]}, {0}, nullptr);
}

bool ForwardRenderer::DoPresent() {
  auto swapchain = swapchain_->GetSwapchain();
  auto swapchain_buf = swapchain_->GetSwapchainBuffer(frame_index_);

  VkCommandBuffer buf = cmd_bufs_present_[frame_index_];

  StartCommandBuffer(buf);
#ifdef DEBUG_LABELS
  VkDebugUtilsLabelEXT label_info{
      .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
      .pNext = nullptr,
      .pLabelName = "Final Image Composition",
      .color = {0, 1, 0, 1}};

  debug_util.vkCmdBeginDebugUtilsLabelEXT(buf, &label_info);
#endif
  VkImageSubresourceRange range{
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .baseMipLevel = 0,
      .levelCount = 1,
      .baseArrayLayer = 0,
      .layerCount = 1,
  };
  blu::core::Image::ImageLayoutTransition(
      buf, ui_img_output[frame_index_]->image,
      VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, range);
  blu::core::Image::ImageLayoutTransition(
      buf, images_render_assist_color[frame_index_]->image,
      VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, range);

  blu::core::Image img{.image = swapchain_buf.image,
                       .view = swapchain_buf.view};
  final_composition->Run(buf, {render_images_descriptor_->sets[frame_index_]},
                         &img, swapchain_->GetWidth(), swapchain_->GetHeight());

  blu::core::Image::ImageLayoutTransition(
      buf, swapchain_buf.image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, range);

#ifdef DEBUG_LABELS
  debug_util.vkCmdEndDebugUtilsLabelEXT(buf);
#endif
  EndCommandBuffer(buf);

  SubmitCommandBuffer({buf}, device_->queues.graphics,
                      {main_frame_semaphore, ui_semaphores[frame_index_]},
                      {semaphore_values.draw_mode_complete, 0},
                      {VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                       VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT},
                      {present_semaphores[frame_index_]}, {0},
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

void ForwardRenderer::StartCommandBuffer(VkCommandBuffer buf) {
  vkResetCommandBuffer(buf, 0);

  VkCommandBufferBeginInfo cmd_buf_begin{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .pNext = nullptr,
      .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
      .pInheritanceInfo = nullptr,
  };

  vkBeginCommandBuffer(buf, &cmd_buf_begin);
}

void ForwardRenderer::EndCommandBuffer(VkCommandBuffer buf) {
  vkEndCommandBuffer(buf);
}

void ForwardRenderer::SubmitCommandBuffer(
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

VkPipelineShaderStageCreateInfo ForwardRenderer::LoadShader(
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

  debug_util.vkSetDebugUtilsObjectNameEXT(device_->GetLogicalDevice(),
                                          &debug_info);
#endif

  return shader_stage;
}

void* __cdecl operator new[](size_t size, const char* name, int flags,
                             unsigned debugFlags, const char* file, int line) {
  return new uint8_t[size];
}