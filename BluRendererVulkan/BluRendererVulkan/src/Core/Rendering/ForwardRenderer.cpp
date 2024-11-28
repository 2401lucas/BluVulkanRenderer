#include "ForwardRenderer.h"

#include <EASTL/array.h>
#include <assimp/postprocess.h>  // Post processing flags

#include <assimp/Importer.hpp>  // C++ importer interface
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
  vertex_buffer_->Destroy(allocator_);
  delete vertex_buffer_;
  index_buffer_->Destroy(allocator_);
  delete index_buffer_;

  for (eastl::vector<VkShaderModule>::iterator it = shader_modules_.begin(),
                                               it_end = shader_modules_.end();
       it != it_end; ++it) {
    vkDestroyShaderModule(device_->GetLogicalDevice(), *it, nullptr);
  }
  
  delete triangle_pipeline_;

  vmaDestroyAllocator(allocator_);
  delete swapchain_;
  delete device_;
  delete instance_;
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
    command_pool_create.queueFamilyIndex =
        device_->queue_family_indicies_.graphics;
    for (size_t i = 0; i < frame_count; i++) {
      vkCreateCommandPool(device_->GetLogicalDevice(), &command_pool_create,
                          nullptr, &graphics_command_pools_[i]);
    }
  }

  // Descriptor Pool Creation
  {
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

  // Mesh Vertex Data
  // Requires: Multiple Chunks of memory instead of one big block
  // Track Buffer used memory, if no memory then allocate new buffer
  {
    vertex_buffer_ = blu::core::Buffer::CreateBuffer(
        device_->GetLogicalDevice(), allocator_, VERTEX_BUFFER_SIZE,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    buffer_infos_.push_back(BufferInfo(vertex_buffer_->device_address,
                                       vertex_buffer_->offset,
                                       vertex_buffer_->size));
  }

  // Model Index Data
  {
    index_buffer_ = blu::core::Buffer::CreateBuffer(
        device_->GetLogicalDevice(), allocator_, INDEX_BUFFER_SIZE,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    buffer_infos_.push_back(BufferInfo(index_buffer_->device_address,
                                       index_buffer_->offset,
                                       index_buffer_->size));
  }

  memcpy(buffer_infos_buffer_->mapped_data, buffer_infos_.data(),
         MAX_BUFFERS_STORAGE * sizeof(BufferInfo));

  // Pipeline Creation
  {
    // Important info for Pipeline creation
    // Attachment Count,
    // Attachment formats
    // Descriptor Set Layouts
    // Shader Sets

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
            .color_attachment_formats = {{*swapchain_->GetColorFormat()}},
            .depth_format = DEPTH_FORMAT,

            .shaders{
                LoadShader("shaders/postProcessing.vert.spv", VK_SHADER_STAGE_VERTEX_BIT),
                LoadShader("shaders/postProcessing.frag.spv",
                           VK_SHADER_STAGE_FRAGMENT_BIT),
            },
        };

    triangle_pipeline_ = new blu::core::rendering::Pipeline(
        device_, triangle_pipeline_create_info);
  }

  // Load Model
  {
    Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile(
        "assets/cube.glTF", aiProcess_Triangulate |
                                aiProcess_JoinIdenticalVertices |
                                aiProcess_SortByPType);
    if (scene == nullptr) {
      std::cerr << importer.GetErrorString() << std::endl;
      return;
    }

    if (scene->HasMeshes()) {
      for (size_t i = 0; i < scene->mNumMeshes; ++i) {
        eastl::vector<uint32_t> indices;

        if (scene->mMeshes[i]->HasFaces()) {
          for (size_t j = 0; j < scene->mMeshes[i]->mNumFaces; j++) {
            for (size_t ind = 0; ind < scene->mMeshes[i]->mFaces[j].mNumIndices;
                 ind++) {
              indices.push_back(scene->mMeshes[i]->mFaces[j].mIndices[ind]);
            }
          }
        }
        if (scene->mMeshes[i]->HasNormals()) {
        }
        if (scene->mMeshes[i]->HasTangentsAndBitangents()) {
        }

        eastl::vector<glm::vec3> vertices;

        for (size_t vI = 0; vI < scene->mMeshes[i]->mNumVertices; vI++) {
          vertices.push_back(glm::vec3(scene->mMeshes[i]->mVertices[vI].x,
                                       scene->mMeshes[i]->mVertices[vI].y,
                                       scene->mMeshes[i]->mVertices[vI].z));
        }

        // Vertex Upload
        {
          vert_count_ = scene->mMeshes[i]->mNumVertices;
          blu::core::Buffer* staging_buffer = blu::core::Buffer::CreateBuffer(
              device_->GetLogicalDevice(), allocator_,
              vert_count_ * sizeof(glm::vec3), VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                  VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
              VMA_ALLOCATION_CREATE_MAPPED_BIT);

          memcpy(staging_buffer->mapped_data, vertices.data(),
                 vert_count_ * sizeof(glm::vec3));

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

          VkBufferCopy copyRegion{
              .srcOffset = 0,
              .dstOffset = 0,
              .size = vert_count_ * sizeof(glm::vec3),
          };

          vkCmdCopyBuffer(copy_cmd_buf, staging_buffer->buffer,
                          vertex_buffer_->buffer, 1, &copyRegion);

          // blu::core::Buffer::BufferMemoryBarrier(copy_cmd_buf,
          //                                        vertex_buffer_->buffer, );

          vkEndCommandBuffer(copy_cmd_buf);

          VkSubmitInfo submitInfo{
              .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
              .commandBufferCount = 1,
              .pCommandBuffers = &copy_cmd_buf,
          };

          vkQueueSubmit(device_->queues.transfer, 1, &submitInfo,
                        VK_NULL_HANDLE);
          vkDeviceWaitIdle(device_->GetLogicalDevice());
          // vkWaitForFences
          staging_buffer->Destroy(allocator_);
          delete staging_buffer;
        }
        // Index Upload
        {
          ind_count_ = indices.size();
          blu::core::Buffer* staging_buffer = blu::core::Buffer::CreateBuffer(
              device_->GetLogicalDevice(), allocator_,
              ind_count_ * sizeof(uint32_t), VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                  VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
              VMA_ALLOCATION_CREATE_MAPPED_BIT);

          memcpy(staging_buffer->mapped_data, indices.data(),
                 ind_count_ * sizeof(uint32_t));

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

          VkBufferCopy copyRegion{
              .srcOffset = 0,
              .dstOffset = 0,
              .size = ind_count_ * sizeof(uint32_t),
          };

          vkCmdCopyBuffer(copy_cmd_buf, staging_buffer->buffer,
                          index_buffer_->buffer, 1, &copyRegion);
          vkEndCommandBuffer(copy_cmd_buf);

          VkSubmitInfo submitInfo{
              .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
              .commandBufferCount = 1,
              .pCommandBuffers = &copy_cmd_buf,
          };

          vkQueueSubmit(device_->queues.transfer, 1, &submitInfo,
                        VK_NULL_HANDLE);
          vkDeviceWaitIdle(device_->GetLogicalDevice());
          // vkWaitForFences
          staging_buffer->Destroy(allocator_);
          delete staging_buffer;
        }

        vkResetCommandPool(device_->GetLogicalDevice(), transfer_command_pool,
                           0);
      }
    }
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

  draw_command_buffers_.resize(frame_count);
  for (size_t i = 0; i < frame_count; i++) {
    VkCommandBufferAllocateInfo command_buffer_alloc{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = graphics_command_pools_[i],
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };
    vkAllocateCommandBuffers(device_->GetLogicalDevice(), &command_buffer_alloc,
                             &draw_command_buffers_[i]);
  }
}

// Update Render Data
// -Matrices
// Render
// Setup Fences & Semaphores
void ForwardRenderer::Render(blu::core::Engine::RenderData render_data) {
  auto swapchain = swapchain_->GetSwapchain();
  vkWaitForFences(device_->GetLogicalDevice(), 1,
                  &in_flight_fences_[frame_index_], VK_TRUE, UINT64_MAX);

  VK_CHECK_RESULT(vkAcquireNextImageKHR(
      device_->GetLogicalDevice(), swapchain, UINT64_MAX,
      image_available_semaphores_[frame_index_], nullptr, &image_index_));
  vkResetFences(device_->GetLogicalDevice(), 1,
                &in_flight_fences_[frame_index_]);

  vkResetCommandPool(device_->GetLogicalDevice(),
                     graphics_command_pools_[frame_index_], 0);

  vkResetCommandPool(device_->GetLogicalDevice(), transfer_command_pool, 0);

  // Update Matrix Buffer
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

  // Render Scene
  {
    auto swapchain_buf = swapchain_->GetSwapchainBuffer(frame_index_);
    auto width = swapchain_->GetWidth();
    auto height = swapchain_->GetHeight();

    // Important info for CPU Rendering
    // VkCommandBuffer
    // Attachment Info
    // Renderer Size
    // Attachment Count

    // Graphics Queue
    {
      VkCommandBuffer draw_cmd_buffer = draw_command_buffers_[frame_index_];

      VkCommandBufferBeginInfo draw_cmd_buffer_begin{
          .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
          .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
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

      blu::core::Image::ImageLayoutTransition(
          draw_cmd_buffer, swapchain_buf.image, VK_IMAGE_LAYOUT_UNDEFINED,
          VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, range);

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

      // For a cube to be rendered
      // Bind Vertices & Indices
      // Bind Descriptor Sets
      // Bind Pipeline
      // Draw

      // vkCmdBindVertexBuffers(draw_cmd_buffer, 0, 1, &vertex_buffer_->buffer,
      // offsets);
      // vkCmdBindIndexBuffer(draw_cmd_buffer, index_buffer_->buffer, 0,
      // VK_INDEX_TYPE_UINT32);
      // vkCmdBindDescriptorSets(draw_cmd_buffer,
      // VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline_layout_, 0, 1,
      //&buffer_infos_descriptor_set_->set, 0, nullptr);
      vkCmdBindPipeline(draw_cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                        *triangle_pipeline_->GetPipeline());
      vkCmdDraw(draw_cmd_buffer, 3, 1, 0, 0);

      vkCmdEndRendering(draw_cmd_buffer);

      blu::core::Image::ImageLayoutTransition(
          draw_cmd_buffer, swapchain_buf.image,
          VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
          VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, range);

      vkEndCommandBuffer(draw_cmd_buffer);

      VkPipelineStageFlags wait_stages[] = {
          VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

      VkSubmitInfo draw_info{
          .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
          .waitSemaphoreCount = 1,
          .pWaitSemaphores = &image_available_semaphores_[frame_index_],
          .pWaitDstStageMask = wait_stages,
          .commandBufferCount = 1,
          .pCommandBuffers = &draw_cmd_buffer,
          .signalSemaphoreCount = 1,
          .pSignalSemaphores = &render_finished_semaphores_[frame_index_],
      };

      VK_CHECK_RESULT(vkQueueSubmit(device_->queues.graphics, 1, &draw_info,
                                    in_flight_fences_[frame_index_]));

      VkPresentInfoKHR present_info = {
          .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
          .pNext = NULL,
          .waitSemaphoreCount = 1,
          .pWaitSemaphores = &render_finished_semaphores_[frame_index_],
          .swapchainCount = 1,
          .pSwapchains = &swapchain,
          .pImageIndices = &frame_index_,
      };

      VK_CHECK_RESULT(
          vkQueuePresentKHR(device_->queues.graphics, &present_info));
    }
  }

  frame_index_++;
  if (frame_index_ >= swapchain_->GetImageCount()) {
    frame_index_ = 0;
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