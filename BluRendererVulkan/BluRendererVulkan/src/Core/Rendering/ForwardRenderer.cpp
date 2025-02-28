#include "ForwardRenderer.h"

#include <cassert>

#include "../External/FileManager.h"
#include "Vulkan/Tools.h"

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
    void* pNextChain = &dynamic_rendering;

    VkPhysicalDeviceDescriptorIndexingFeaturesEXT descriptor_indexing{
        .sType =
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT,
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

    device_ = new blu::core::Device(instance_, device_extensions, pNextChain);
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
    vkDestroySemaphore(device_->GetLogicalDevice(),
                       image_available_semaphores_[i], nullptr);
    vkDestroySemaphore(device_->GetLogicalDevice(),
                       render_finished_semaphores_[i], nullptr);
    vkDestroyFence(device_->GetLogicalDevice(), in_flight_fences_[i], nullptr);
  }
  delete graphics_command_pools_;

  vkDestroyCommandPool(device_->GetLogicalDevice(), transfer_command_pool,
                       nullptr);
  vkDestroyDescriptorPool(device_->GetLogicalDevice(), descriptor_pool_,
                          nullptr);
  depth_stencil_image_->Destroy(device_->GetLogicalDevice(), allocator_);
  delete depth_stencil_image_;
  buffer_infos_buffer_->Destroy(allocator_);
  delete buffer_infos_buffer_;
  buffer_infos_descriptor_set_->Destroy(device_->GetLogicalDevice());
  delete buffer_infos_descriptor_set_;
  matrices_buffer_->Destroy(allocator_);
  delete matrices_buffer_;

  for (auto& buf : vertex_buffers_) {
    buf->Destroy(allocator_);
    delete buf;
  }
  for (auto& buf : index_buffers_) {
    buf->Destroy(allocator_);
    delete buf;
  }
  for (auto& buf : normal_buffers_) {
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

  delete triangle_pipeline_;
  delete cube_pipeline_;

  vmaDestroyAllocator(allocator_);
  delete swapchain_;
  delete device_;
  delete instance_;
}

// For now each model will get it's own buffer, eventually this will be replaced
// to use large, shared buffers
// For now- NOT ASYNC
uint32_t ForwardRenderer::LoadModel(eastl::string filepath) {
  if (loaded_model_indices_.find(filepath) != loaded_model_indices_.end()) {
    return loaded_model_indices_[filepath];
  }
  uint32_t model_index = loaded_models_.size();
  auto new_model = blu::core::components::Model(filepath);

  if (new_model.GetVertexData() == nullptr) {
    return -1;
  }

  loaded_models_.push_back(new_model);
  auto& model = loaded_models_[model_index];
  loaded_model_indices_[filepath] = model_index;

  blu::core::Buffer* vertex_buffer_ = blu::core::Buffer::CreateBuffer(
      device_->GetLogicalDevice(), allocator_, model.GetVertexDataSize(),
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
          VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
          VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  buffer_infos_.push_back(BufferInfo(vertex_buffer_->device_address,
                                     vertex_buffer_->offset,
                                     vertex_buffer_->size));

  blu::core::Buffer* index_buffer_ = blu::core::Buffer::CreateBuffer(
      device_->GetLogicalDevice(), allocator_, model.GetIndexDataSize(),
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
          VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
          VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  buffer_infos_.push_back(BufferInfo(index_buffer_->device_address,
                                     index_buffer_->offset,
                                     index_buffer_->size));

  blu::core::Buffer* normal_buffer_ = blu::core::Buffer::CreateBuffer(
      device_->GetLogicalDevice(), allocator_, model.GetNormalDataSize(),
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
          VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
          VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  buffer_infos_.push_back(BufferInfo(normal_buffer_->device_address,
                                     normal_buffer_->offset,
                                     normal_buffer_->size));

  memcpy(buffer_infos_buffer_->mapped_data, buffer_infos_.data(),
         buffer_infos_.size() * sizeof(BufferInfo));

  VkCommandBufferAllocateInfo alloc_info{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool = transfer_command_pool,
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
      device_->GetLogicalDevice(), allocator_, vertex_buffer_, 0, copy_cmd_buf,
      model.GetVertexData(), sizeof(model.GetVertexData()), 0,
      vert_staging_buffer);
  blu::core::Buffer* normal_staging_buffer;
  blu::core::Buffer::UploadToBuffer(
      device_->GetLogicalDevice(), allocator_, normal_buffer_, 0, copy_cmd_buf,
      model.GetNormalData(), sizeof(model.GetNormalData()), 0,
      normal_staging_buffer);
  blu::core::Buffer* index_staging_buffer;
  blu::core::Buffer::UploadToBuffer(
      device_->GetLogicalDevice(), allocator_, index_buffer_, 0, copy_cmd_buf,
      model.GetIndexData(), sizeof(model.GetIndexData()), 0,
      index_staging_buffer);

  model_indices_.push_back({
      .pipeline_index = 0,
      .vert_count = model.GetVertexCount(),
      .ind_count = model.GetIndexCount(),
      .mesh_vert_buf_index = static_cast<uint32_t>(vertex_buffers_.size()),
      .mesh_norm_buf_index = static_cast<uint32_t>(normal_buffers_.size()),
      .mesh_ind_buf_index = static_cast<uint32_t>(index_buffers_.size()),
  });

  vertex_buffers_.push_back(vertex_buffer_);
  normal_buffers_.push_back(normal_buffer_);
  index_buffers_.push_back(index_buffer_);

  vkEndCommandBuffer(copy_cmd_buf);

  VkSubmitInfo submitInfo{
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .commandBufferCount = 1,
      .pCommandBuffers = &copy_cmd_buf,
  };

  vkQueueSubmit(device_->queues.transfer, 1, &submitInfo, VK_NULL_HANDLE);

  // TODO: REMOVE
  vkDeviceWaitIdle(device_->GetLogicalDevice());
  vert_staging_buffer->Destroy(allocator_);
  delete vert_staging_buffer;
  normal_staging_buffer->Destroy(allocator_);
  delete normal_staging_buffer;
  index_staging_buffer->Destroy(allocator_);
  delete index_staging_buffer;
}

void ForwardRenderer::Prepare() {
  auto frame_count = swapchain_->GetImageCount();
  // Command Pool Creation
  {
    VkCommandPoolCreateInfo command_pool_create{
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .queueFamilyIndex = device_->queue_family_indicies_.transfer,
    };
    vkCreateCommandPool(device_->GetLogicalDevice(), &command_pool_create,
                        nullptr, &transfer_command_pool);

    graphics_command_pools_ = new VkCommandPool[frame_count];
    draw_command_buffers = new VkCommandBuffer[frame_count];

    command_pool_create.queueFamilyIndex =
        device_->queue_family_indicies_.graphics;
    command_pool_create.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    for (size_t i = 0; i < frame_count; i++) {
      vkCreateCommandPool(device_->GetLogicalDevice(), &command_pool_create,
                          nullptr, &graphics_command_pools_[i]);

      VkCommandBufferAllocateInfo command_buffer_alloc_info{
          .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
          .pNext = nullptr,
          .commandPool = graphics_command_pools_[i],
          .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
          .commandBufferCount = 1,
      };
      vkAllocateCommandBuffers(device_->GetLogicalDevice(),
                               &command_buffer_alloc_info,
                               &draw_command_buffers[i]);
    }
  }

  // Descriptor Pool Creation
  {
    // One VK_DESCRIPTOR_TYPE_STORAGE_BUFFER for holding references to Buffer
    // Device Pointers
    eastl::vector<VkDescriptorPoolSize> pool_sizes{
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1},
    };

    VkDescriptorPoolCreateInfo descriptor_pool_create{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .flags = 0,
        .maxSets = 1,
        .poolSizeCount = static_cast<uint32_t>(pool_sizes.size()),
        .pPoolSizes = pool_sizes.data(),
    };

    VK_CHECK_RESULT(vkCreateDescriptorPool(device_->GetLogicalDevice(),
                                           &descriptor_pool_create, nullptr,
                                           &descriptor_pool_));
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
        .descriptorPool = descriptor_pool_,
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

  // Matrix Buffer Creation
  {
    matrices_buffer_ = blu::core::Buffer::CreateBuffer(
        device_->GetLogicalDevice(), allocator_, sizeof(glm::mat4) * 4,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
            VK_MEMORY_PROPERTY_HOST_CACHED_BIT,
        VMA_ALLOCATION_CREATE_MAPPED_BIT);

    buffer_infos_.push_back(BufferInfo(matrices_buffer_->device_address,
                                       matrices_buffer_->offset,
                                       matrices_buffer_->size));
  }

  memcpy(buffer_infos_buffer_->mapped_data, buffer_infos_.data(),
         MAX_BUFFERS_STORAGE * sizeof(BufferInfo));

  // Pipeline Creation
  {
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
            .depth_stencil_depth_compare_op = VK_COMPARE_OP_GREATER,
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
        .depth_stencil_depth_test = VK_TRUE,
        .depth_stencil_depth_write = VK_TRUE,
        .depth_stencil_depth_compare_op = VK_COMPARE_OP_GREATER,
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
                {0, 3 * sizeof(float), VK_VERTEX_INPUT_RATE_VERTEX},
                //{1, 3 * sizeof(float), VK_VERTEX_INPUT_RATE_VERTEX},
            },
        .vertex_input_attributes =
            {
                {0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0},
                //{1, 1, VK_FORMAT_R32G32B32_SFLOAT, 0},
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
      vkCreateFence(device_->GetLogicalDevice(), &fence_info, nullptr,
                    &in_flight_fences_[i]);
    }
  }
  matrices_.resize(4);
}

// Update Render Data
// -Matrices
// Render
// Setup Fences & Semaphores
void ForwardRenderer::Render(RenderData render_data) {
  vkWaitForFences(device_->GetLogicalDevice(), 1,
                  &in_flight_fences_[frame_index_], VK_TRUE, UINT64_MAX);

  auto swapchain = swapchain_->GetSwapchain();
  // Handle Window Resize
  VK_CHECK_RESULT(vkAcquireNextImageKHR(
      device_->GetLogicalDevice(), swapchain, UINT64_MAX,
      image_available_semaphores_[frame_index_], nullptr, &image_index_));

  vkResetFences(device_->GetLogicalDevice(), 1,
                &in_flight_fences_[frame_index_]);

  VkCommandBuffer draw_cmd_buffer = draw_command_buffers[frame_index_];
  vkResetCommandBuffer(draw_cmd_buffer, 0);

  // Update Data
  {
    matrices_[0] = render_data.matrices[0] * render_data.matrices[1];
    matrices_[1] = render_data.matrices[0];
    matrices_[2] = render_data.matrices[1];

    // ... USE ITERATOR
    matrices_[3] = render_data.matrices[2];
    // ...

    memcpy(matrices_buffer_->mapped_data, matrices_.data(),
           matrices_.size() * sizeof(glm::mat4));
  }

  // Render
  {
    auto swapchain_buf = swapchain_->GetSwapchainBuffer(image_index_);
    auto width = swapchain_->GetWidth();
    auto height = swapchain_->GetHeight();
    // Graphics Queue
    {
      VkCommandBufferAllocateInfo graphics_cmd_buf_alloc_info{
          .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
          .pNext = nullptr,
          .commandPool = graphics_command_pools_[frame_index_],
          .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
          .commandBufferCount = 1,
      };

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
      clear_values[0].color = {{0.0f, 0.0f, 1.0f, 1.0f}};
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

      // vkCmdBindVertexBuffers(draw_cmd_buffer, 0, 1,
      // &vertex_buffers_[0]->buffer,
      //                        offsets);
      // vkCmdBindIndexBuffer(draw_cmd_buffer, index_buffers_[0]->buffer, 0,
      //                      VK_INDEX_TYPE_UINT32);

      // vkCmdBindDescriptorSets(draw_cmd_buffer,
      // VK_PIPELINE_BIND_POINT_GRAPHICS,
      //                         *cube_pipeline_->GetPipelineLayout(), 0, 1,
      //                         &buffer_infos_descriptor_set_->set, 0,
      //                         nullptr);

      // vkCmdBindPipeline(draw_cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
      //                   *cube_pipeline_->GetPipeline());
      // vkCmdDrawIndexed(draw_cmd_buffer, 12, 1, 0, 0, 0);
      vkCmdEndRendering(draw_cmd_buffer);

      blu::core::Image::ImageLayoutTransition(
          draw_cmd_buffer, swapchain_buf.image,
          VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
          VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, range);

      vkEndCommandBuffer(draw_cmd_buffer);

      VkSemaphore draw_wait_semaphores[] = {
          image_available_semaphores_[frame_index_]};

      VkPipelineStageFlags draw_wait_stages[] = {
          VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

      VkSemaphore draw_signal_semaphores[] = {
          render_finished_semaphores_[frame_index_]};

      VkSubmitInfo draw_info{
          .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
          .waitSemaphoreCount = 1,
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

      VK_CHECK_RESULT(
          vkQueuePresentKHR(device_->queues.graphics, &present_info));
    }
  }

  frame_index_ = (frame_index_ + 1) % swapchain_->GetImageCount();
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