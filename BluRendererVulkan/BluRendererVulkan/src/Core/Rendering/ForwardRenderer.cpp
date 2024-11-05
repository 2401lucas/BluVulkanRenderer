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
  // Descriptor Pool Creation
  {
    eastl::vector<VkDescriptorPoolSize> pool_sizes{
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1},
    };

    VkDescriptorPoolCreateInfo descriptor_pool_ci{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .flags = 0,
        .maxSets = 1,
        .poolSizeCount = pool_sizes.size(),
        .pPoolSizes = pool_sizes.data(),
    };

    VK_CHECK_RESULT(vkCreateDescriptorPool(device_->GetLogicalDevice(),
                                           &descriptor_pool_ci, nullptr,
                                           &descriptor_pool_));
  }

  // Buffer Infos Buffer & Descriptor Creation
  {
    buffer_infos_buffer_ = new blu::core::Buffer();

    VkBufferCreateInfo buf_ci{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = sizeof(BufferInfo) * MAX_BUFFERS,
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
    matrices_buffer_ = new blu::core::Buffer();
    VkBufferCreateInfo buf_ci{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = sizeof(glm::mat4) * 4,
        .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                 VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
    };

    VmaAllocationCreateInfo alloc_ci{
        .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT,
        .requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        .preferredFlags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
                          VK_MEMORY_PROPERTY_HOST_CACHED_BIT,
    };

    VmaAllocationInfo allocInfo;
    vmaCreateBuffer(allocator_, &buf_ci, &alloc_ci, &matrices_buffer_->buffer,
                    &matrices_buffer_->alloc, &allocInfo);

    matrices_buffer_->size = buf_ci.size;
    matrices_buffer_->mapped_data = allocInfo.pMappedData;

    VkBufferDeviceAddressInfo info{
        .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        .buffer = matrices_buffer_->buffer};

    matrices_buffer_->device_address =
        vkGetBufferDeviceAddress(device_->GetLogicalDevice(), &info);

    buffer_infos_.push_back(BufferInfo(matrices_buffer_->device_address,
                                       matrices_buffer_->offset,
                                       matrices_buffer_->size));
  }
  
  // Mesh Vertex Data
  // Requires: Multiple Chunks of memory instead of one big block
  // Track Buffer used memory, if no memory then allocate new buffer
  {
    
  }

  // Model Index Data
  {
    model_indices_buffer_ = new blu::core::Buffer();
    VkBufferCreateInfo buf_ci{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = sizeof(glm::mat4) * 4,
        .usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                 VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
    };

    VmaAllocationCreateInfo alloc_ci{
        .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT,
        .requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    };

    VmaAllocationInfo allocInfo;
    vmaCreateBuffer(allocator_, &buf_ci, &alloc_ci,
                    &model_indices_buffer_->buffer,
                    &model_indices_buffer_->alloc, &allocInfo);

    model_indices_buffer_->size = buf_ci.size;
    model_indices_buffer_->mapped_data = allocInfo.pMappedData;

    VkBufferDeviceAddressInfo info{
        .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        .buffer = model_indices_buffer_->buffer};

    model_indices_buffer_->device_address =
        vkGetBufferDeviceAddress(device_->GetLogicalDevice(), &info);

    buffer_infos_.push_back(BufferInfo(model_indices_buffer_->device_address,
                                       model_indices_buffer_->offset,
                                       model_indices_buffer_->size));
  }

  memcpy(buffer_infos_buffer_->mapped_data, buffer_infos_.data(),
         buffer_infos_.size() * sizeof(BufferInfo));
}

void ForwardRenderer::Render(blu::core::Engine::RenderData render_data) {
  matrices_.resize(4);
  matrices_[0] = render_data.matrices[0] * render_data.matrices[1];
  matrices_[1] = render_data.matrices[0];
  matrices_[2] = render_data.matrices[1];

  // ... USE ITERATOR
  matrices_[3] = render_data.matrices[2];
  // ...

  // Use Staging Buffer
  memcpy(nullptr /*pointer to GPU memory*/, matrices_.data(),
         matrices_.size() * sizeof(glm::mat4));
}

  blu::core::Buffer* CreateVertexBuffer(VkDeviceSize size, VkBufferUsageFlags usage, 
      VmaAllocationCreateFlags flags, VkMemoryPropertyFlags required_flags, ){
    blu::core::Buffer* new_buffer = new blu::core::Buffer();
    VkBufferCreateInfo buf_ci{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = VERTEX_BUFFER_SIZE,
        .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                 VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
    };

    VmaAllocationCreateInfo alloc_ci{
        .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT,
        .requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    };

    VmaAllocationInfo allocInfo;
    vmaCreateBuffer(allocator_, &buf_ci, &alloc_ci,
                    &vertex_buffer_->buffer,
                    &vertex_buffer_->alloc, &allocInfo);

    vertex_buffer_->size = buf_ci.size;
    vertex_buffer_->mapped_data = allocInfo.pMappedData;

    VkBufferDeviceAddressInfo info{
        .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        .buffer = vertex_buffer_->buffer};

    vertex_buffer_->device_address =
        vkGetBufferDeviceAddress(device_->GetLogicalDevice(), &info);

    buffer_infos_.push_back(BufferInfo(vertex_buffer_->device_address,
                                       vertex_buffer_->offset,
                                       vertex_buffer_->size));

  
  }