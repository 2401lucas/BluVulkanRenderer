#include "ForwardRenderer.h"

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
        .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO_KHR,
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

    delete pNextChain;
  }
  // VkSwapchain Creation
  {
    swapchain_ = new blu::core::Swapchain(instance_, device_, window_);
    swapchain_->Create(false, false);
  }
}

ForwardRenderer::~ForwardRenderer() {
  delete swapchain_;
  delete device_;
  delete instance_;
}

void ForwardRenderer::Prepare() {
  frame_index_ = 0;
  auto frame_count = swapchain_->GetImageCount();
  // Command Pool Creation
  {
    graphics_command_pools_ = new VkCommandPool[frame_count];

    for (size_t i = 0; i < frame_count; i++) {
      VkCommandPoolCreateInfo command_pool_create{
          .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
          .queueFamilyIndex = 0,
      };

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
        .poolSizeCount = pool_sizes.size(),
        .pPoolSizes = pool_sizes.data(),
    };

    VK_CHECK_RESULT(vkCreateDescriptorPool(device_->GetLogicalDevice(),
                                           &descriptor_pool_create, nullptr,
                                           &descriptor_pool_));
  }

  // Buffer Infos Buffer & Descriptor Creation
  {
    buffer_infos_buffer_ = new blu::core::Buffer();

    VkBufferCreateInfo buf_ci{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = sizeof(BufferInfo) * MAX_BUFFERS_STORAGE,
        .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                 VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
    };

    VmaAllocationCreateInfo alloc_ci{
        .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT,
        .requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                         VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        .preferredFlags = VK_MEMORY_PROPERTY_HOST_CACHED_BIT,
    };

    VmaAllocationInfo alloc_info;
    vmaCreateBuffer(allocator_, &buf_ci, &alloc_ci, &matrices_buffer_->buffer,
                    &matrices_buffer_->alloc, &alloc_info);

    matrices_buffer_->size = buf_ci.size;
    matrices_buffer_->mapped_data = alloc_info.pMappedData;

    VkBufferDeviceAddressInfo info{
        .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        .buffer = matrices_buffer_->buffer};

    matrices_buffer_->device_address =
        vkGetBufferDeviceAddress(device_->GetLogicalDevice(), &info);

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
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    buffer_infos_.push_back(BufferInfo(vertex_buffer_->device_address,
                                       vertex_buffer_->offset,
                                       vertex_buffer_->size));
  }

  // Model Index Data
  {
    index_buffer_ = blu::core::Buffer::CreateBuffer(
        device_->GetLogicalDevice(), allocator_, INDEX_BUFFER_SIZE,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    buffer_infos_.push_back(BufferInfo(index_buffer_->device_address,
                                       index_buffer_->offset,
                                       index_buffer_->size));
  }

  memcpy(buffer_infos_buffer_->mapped_data, buffer_infos_.data(),
         buffer_infos_.size() * sizeof(BufferInfo));

  // Pipeline Creation
  {
    // Important info for Pipeline creation
    // Attachment Count,
    // Attachment formats
    // Descriptor Set Layouts
    VkPipelineLayoutCreateInfo pipeline_layout_create{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &buffer_infos_descriptor_set_->layout,
    };

    vkCreatePipelineLayout(device_->GetLogicalDevice(), &pipeline_layout_create,
                           nullptr, &graphics_pipeline_layout_);

    VkPipelineRenderingCreateInfo pipeline_create{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = swapchain_->GetColorFormat(),
        .depthAttachmentFormat = VK_FORMAT_D24_UNORM_S8_UINT,
        .stencilAttachmentFormat = VK_FORMAT_D24_UNORM_S8_UINT,
    };

    VkGraphicsPipelineCreateInfo graphics_create{
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = &pipeline_create,
        .renderPass = VK_NULL_HANDLE,
    };

    VK_CHECK_RESULT(vkCreateGraphicsPipelines(device_->GetLogicalDevice(),
                                              nullptr, 1, &graphics_create,
                                              nullptr, &graphics_pipeline_));
  }
}

// Update Render Data
// -Matrices
// Render
void ForwardRenderer::Render(blu::core::Engine::RenderData render_data) {
  frame_index_++;
  if (frame_index_ >= swapchain_->GetImageCount()) {
    frame_index_ = 0;
  }

  vkResetCommandPool(device_->GetLogicalDevice(),
                     graphics_command_pools_[frame_index_], 0);

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
    auto width = swapchain_->GetWidth();
    auto height = swapchain_->GetHeight();

    // Important info for CPU Rendering
    // VkCommandBuffer
    // Attachment Info
    // Renderer Size
    // Attachment Count
    VkCommandBufferAllocateInfo command_buffer_alloc{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = graphics_command_pools_[frame_index_],
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };

    VkCommandBuffer draw_cmd_buffer;

    vkAllocateCommandBuffers(device_->GetLogicalDevice(), &command_buffer_alloc,
                             &draw_cmd_buffer);

    VkCommandBufferBeginInfo draw_cmd_buffer_begin{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };

    vkBeginCommandBuffer(draw_cmd_buffer, &draw_cmd_buffer_begin);

    //Perform Layout Transition to VK_UNKNOWN_LAYOUT

    VkRenderingAttachmentInfo color_attachment_info{
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView = swapchain_->GetSwapchainBuffer(frame_index_),
    };

    VkRenderingAttachmentInfo depth_attachment_info{
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
    };

    VkRenderingInfo render_info{
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .renderArea = {VkOffset2D(), width, height},
        .colorAttachmentCount = 1,
        .pColorAttachments = &color_attachment_info,
    };

    vkCmdBeginRenderingKHR(draw_cmd_buffer, &render_info);

    VkViewport viewport{
        .width = width,
        .height = height,
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

    vkCmdBindDescriptorSets(draw_cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            graphics_pipeline_layout_, 0, 1,
                            &buffer_infos_descriptor_set_->set, 0, nullptr);
    vkCmdBindPipeline(draw_cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                      graphics_pipeline_);
    vkCmdDraw(draw_cmd_buffer, 3, 1, 0, 0);

    vkCmdEndRenderingKHR(draw_cmd_buffer);
  }
}