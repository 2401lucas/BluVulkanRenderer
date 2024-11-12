#include "ForwardRenderer.h"

#include <EASTL/array.h>
#include <assimp/postprocess.h>  // Post processing flags
#include <assimp/scene.h>        // Output data structure

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

    instance_ =
        new blu::core::Instance("Forward Renderer", true,
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
  for (size_t i = 0; i < swapchain_->GetImageCount(); i++) {
    vkDestroyCommandPool(device_->GetLogicalDevice(),
                         graphics_command_pools_[i], nullptr);
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
  vkDestroyPipelineLayout(device_->GetLogicalDevice(),
                          graphics_pipeline_layout_, nullptr);
  vkDestroyPipeline(device_->GetLogicalDevice(), graphics_pipeline_, nullptr);

  vmaDestroyAllocator(allocator_);
  delete swapchain_;
  delete device_;
  delete instance_;
}

void ForwardRenderer::Prepare() {
  frame_index_ = 0;
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

    VkPipelineInputAssemblyStateCreateInfo input_assembly_state{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .flags = 0,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE,
    };

    VkPipelineRasterizationStateCreateInfo rasterization_state{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .flags = 0,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
    };

    VkPipelineColorBlendAttachmentState blend_attachment_state{
        .blendEnable = VK_FALSE,
        .colorWriteMask = 0xf /*RGBA*/,
    };

    VkPipelineColorBlendStateCreateInfo color_blend_state{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &blend_attachment_state,
    };

    VkPipelineDepthStencilStateCreateInfo depth_stencil_state{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = VK_FALSE,
        .depthWriteEnable = VK_FALSE,
        .depthCompareOp = VK_COMPARE_OP_GREATER,
        .back{
            .compareOp = VK_COMPARE_OP_ALWAYS,
        },
    };
    depth_stencil_state.front = depth_stencil_state.back;

    VkPipelineViewportStateCreateInfo viewport_state{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .flags = 0,
        .viewportCount = 1,
        .scissorCount = 1,
    };

    VkPipelineMultisampleStateCreateInfo multisample_state{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .flags = 0,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
    };

    eastl::array<VkDynamicState, 2> dynamic_state_enables{
        VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamic_state{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .flags = 0,
        .dynamicStateCount = 2,
        .pDynamicStates = dynamic_state_enables.data(),
    };

    // Binding description
    eastl::array<VkVertexInputBindingDescription, 1> vertex_input_bindings{
        VkVertexInputBindingDescription(0, sizeof(Vertex),
                                        VK_VERTEX_INPUT_RATE_VERTEX)};

    // Attribute descriptions
    eastl::array<VkVertexInputAttributeDescription, 1> vertex_input_attributes{

        VkVertexInputAttributeDescription(0, 0, VK_FORMAT_R32G32B32_SFLOAT,
                                          0)  // Position
/*      ,  VkVertexInputAttributeDescription(0, 1, VK_FORMAT_R32G32_SFLOAT,
                                          sizeof(float) * 3),*/  // UV
    };

    VkPipelineLayoutCreateInfo pipeline_layout_create{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &buffer_infos_descriptor_set_->layout,
    };

    VkPipelineVertexInputStateCreateInfo vertex_input_state{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount =
            static_cast<uint32_t>(vertex_input_bindings.size()),
        .pVertexBindingDescriptions = vertex_input_bindings.data(),
        .vertexAttributeDescriptionCount =
            static_cast<uint32_t>(vertex_input_attributes.size()),
        .pVertexAttributeDescriptions = vertex_input_attributes.data(),
    };

    vkCreatePipelineLayout(device_->GetLogicalDevice(), &pipeline_layout_create,
                           nullptr, &graphics_pipeline_layout_);

    VkPipelineRenderingCreateInfo pipeline_create{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = swapchain_->GetColorFormat(),
        .depthAttachmentFormat = DEPTH_FORMAT,
        //.stencilAttachmentFormat = DEPTH_FORMAT,
    };

    eastl::array<VkPipelineShaderStageCreateInfo, 2> shaders = {
        LoadShader("shaders/cube.vert.spv", VK_SHADER_STAGE_VERTEX_BIT),
        LoadShader("shaders/cube.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT),
    };

    VkGraphicsPipelineCreateInfo graphics_create{
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = &pipeline_create,
        .stageCount = 2,
        .pStages = shaders.data(),
        .pVertexInputState = &vertex_input_state,
        .pInputAssemblyState = &input_assembly_state,
        .pViewportState = &viewport_state,
        .pRasterizationState = &rasterization_state,
        .pMultisampleState = &multisample_state,
        .pDepthStencilState = &depth_stencil_state,
        .pColorBlendState = &color_blend_state,
        .pDynamicState = &dynamic_state,
        .layout = graphics_pipeline_layout_,
        .renderPass = VK_NULL_HANDLE,
    };

    // ^ For Skybox
    // Enable depth test and write
    graphics_create.stageCount = 2;
    graphics_create.pStages = shaders.data();
    depth_stencil_state.depthWriteEnable = VK_TRUE;
    depth_stencil_state.depthTestEnable = VK_TRUE;
    rasterization_state.cullMode = VK_CULL_MODE_FRONT_BIT;

    VK_CHECK_RESULT(vkCreateGraphicsPipelines(device_->GetLogicalDevice(),
                                              nullptr, 1, &graphics_create,
                                              nullptr, &graphics_pipeline_));
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

        // Vertex Upload
        {
          vert_count_ = scene->mMeshes[i]->mNumVertices;
          blu::core::Buffer* staging_buffer = blu::core::Buffer::CreateBuffer(
              device_->GetLogicalDevice(), allocator_,
              scene->mMeshes[i]->mNumVertices * sizeof(aiVector3D),
              VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                  VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
              VMA_ALLOCATION_CREATE_MAPPED_BIT);

          memcpy(staging_buffer->mapped_data, scene->mMeshes[i]->mVertices,
                 scene->mMeshes[i]->mNumVertices * sizeof(aiVector3D));

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
              .size = scene->mMeshes[i]->mNumVertices * sizeof(aiVector3D),
          };

          vkCmdCopyBuffer(copy_cmd_buf, staging_buffer->buffer,
                          vertex_buffer_->buffer, 1, &copyRegion);
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
              indices.size() * sizeof(uint32_t),
              VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                  VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
              VMA_ALLOCATION_CREATE_MAPPED_BIT);

          memcpy(staging_buffer->mapped_data, indices.data(),
                 indices.size() * sizeof(uint32_t));

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
              .size = indices.size() * sizeof(uint32_t),
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
}

// Update Render Data
// -Matrices
// Render
// Setup Fences & Semaphores
void ForwardRenderer::Render(blu::core::Engine::RenderData render_data) {
  /*frame_index_++;
  if (frame_index_ >= swapchain_->GetImageCount()) {
    frame_index_ = 0;
  }*/
  auto swapchain = swapchain_->GetSwapchain();
  frame_index_ = 0;
  vkWaitForFences(device_->GetLogicalDevice(), 1,
                  &in_flight_fences_[frame_index_], VK_TRUE, UINT64_MAX);

  vkAcquireNextImageKHR(device_->GetLogicalDevice(), swapchain,
                        UINT64_MAX, image_available_semaphores_[frame_index_],
                        nullptr, &frame_index_);
  vkResetFences(device_->GetLogicalDevice(), 1,
                &in_flight_fences_[frame_index_]);

  vkResetCommandPool(device_->GetLogicalDevice(),
                     graphics_command_pools_[frame_index_], 0);

  vkResetCommandPool(device_->GetLogicalDevice(), transfer_command_pool, 0);

  // Update Matrix Buffer
  {
    matrices_.resize(4);
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
      VkCommandBuffer draw_cmd_buffer;

      VkCommandBufferAllocateInfo command_buffer_alloc{
          .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
          .commandPool = graphics_command_pools_[frame_index_],
          .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
          .commandBufferCount = 1,
      };
      vkAllocateCommandBuffers(device_->GetLogicalDevice(),
                               &command_buffer_alloc, &draw_cmd_buffer);

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
          draw_cmd_buffer, swapchain_buf.image,
          VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
          VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0,
          VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
          VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, range);

      blu::core::Image::ImageLayoutTransition(
          draw_cmd_buffer, depth_stencil_image_->image,
          VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
          depth_range);

      eastl::array<VkClearValue, 2> clear_values{};
      clear_values[0].color = {{0.0f, 0.0f, 0.0f, 0.0f}};
      clear_values[1].depthStencil = {0.0f, 0};

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
      vkCmdBindVertexBuffers(draw_cmd_buffer, 0, 1, &vertex_buffer_->buffer,
                             offsets);
      vkCmdBindIndexBuffer(draw_cmd_buffer, index_buffer_->buffer, 0,
                           VK_INDEX_TYPE_UINT32);
      vkCmdBindDescriptorSets(draw_cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                              graphics_pipeline_layout_, 0, 1,
                              &buffer_infos_descriptor_set_->set, 0, nullptr);
      vkCmdBindPipeline(draw_cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                        graphics_pipeline_);
      vkCmdDraw(draw_cmd_buffer, 3, 1, 0, 0);

      vkCmdEndRendering(draw_cmd_buffer);

      //blu::core::Image::ImageLayoutTransition(
      //    draw_cmd_buffer, swapchain_buf.image,
      //    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      //    VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, range);

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

      vkQueuePresentKHR(device_->queues.graphics, &present_info);
    }
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