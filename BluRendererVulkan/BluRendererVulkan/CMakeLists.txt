# CMakeList.txt : CMake project for BluRendererVulkan, include source and define
# project specific logic here.
cmake_minimum_required(VERSION 3.11)

find_package(Vulkan)
find_package(glfw3 CONFIG REQUIRED)

# Add source to this project's executable.
add_executable (BluRendererVulkan ${src_files} ${include_files} "src/BluRendererVulkan.cpp"  "src/FileManager.cpp" "include/FileManager.h"   "src/Window/WindowManager.cpp" "src/Window/WindowManager.h" "src/VkInstance/VulkanInstance.cpp" "src/VkInstance/VulkanInstance.h" "src/Scene/Scene.h" "src/Model/Model.h" "src/Model/Model.cpp" "src/Image/Image.h" "src/Image/Image.cpp" "src/Device/Device.h" "src/Device/Device.cpp" "include/Settings/VkLayerConfig.h" "src/Swapchain/Swapchain.h" "src/Swapchain/Swapchain.cpp" "src/Renderer/RenderManager.cpp" "src/Renderer/RenderManager.h" "src/Command/CommandPool.h" "src/Command/CommandPool.cpp" "src/RenderPass/RenderPass.h" "src/RenderPass/RenderPass.cpp" "src/Descriptors/Descriptor.h"  "src/Descriptors/Types/UBO/UBO.h" "src/Descriptors/Types/UBO/UBO.cpp" "src/Buffer/Buffer.h" "src/Buffer/Buffer.cpp" "src/Descriptors/Descriptor.cpp" "src/Pipeline/Pipeline.h" "src/Pipeline/Pipeline.cpp" "src/Pipeline/GraphicsPipeline.h" "src/Pipeline/GraphicsPipeline.cpp" "src/Buffer/ModelBufferManager.h" "src/Buffer/ModelBufferManager.cpp" "src/Descriptors/DescriptorPool.h" "src/Descriptors/DescriptorPool.cpp" "src/Descriptors/DescriptorSetManager.h" "src/Descriptors/DescriptorSetManager.cpp" "src/BluRendererVulkan.h" "include/RenderConst.h")

target_include_directories(BluRendererVulkan PRIVATE source_dir, include_dir)
target_include_directories(BluRendererVulkan PRIVATE ${Vulkan_INCLUDE_DIRS})
target_link_libraries(BluRendererVulkan PUBLIC ${Vulkan_LIBRARIES} glfw)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET BluRendererVulkan PROPERTY CXX_STANDARD 20)
endif()