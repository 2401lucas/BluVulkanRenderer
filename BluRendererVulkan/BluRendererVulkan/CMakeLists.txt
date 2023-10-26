# CMakeList.txt : CMake project for BluRendererVulkan, include source and define
# project specific logic here.
cmake_minimum_required(VERSION 3.11)

find_package(Vulkan)
find_package(glfw3 CONFIG REQUIRED)

# Add source to this project's executable.
add_executable (BluRendererVulkan ${src_files} ${include_files} "src/BluRendererVulkan.cpp"  "src/Engine/FileManagement/FileManager.cpp" "src/Engine/FileManagement/FileManager.h"   "src/Render/Window/WindowManager.cpp" "src/Render/Window/WindowManager.h" "src/Render/VkInstance/VulkanInstance.cpp" "src/Render/VkInstance/VulkanInstance.h" "src/Engine/Scene/Scene.h" "src/Render/Model/Model.h" "src/Render/Model/Model.cpp" "src/Render/Image/Image.h" "src/Render/Image/Image.cpp" "src/Render/Device/Device.h" "src/Render/Device/Device.cpp" "include/Settings/VkLayerConfig.h" "src/Render/Swapchain/Swapchain.h" "src/Render/Swapchain/Swapchain.cpp" "src/Render/Renderer/RenderManager.cpp" "src/Render/Renderer/RenderManager.h" "src/Render/Command/CommandPool.h" "src/Render/Command/CommandPool.cpp" "src/Render/RenderPass/RenderPass.h" "src/Render/RenderPass/RenderPass.cpp" "src/Render/Descriptors/Descriptor.h"  "src/Render/Descriptors/Types/UBO/UBO.h" "src/Render/Descriptors/Types/UBO/UBO.cpp" "src/Render/Buffer/Buffer.h" "src/Render/Buffer/Buffer.cpp" "src/Render/Descriptors/Descriptor.cpp" "src/Render/Pipeline/Pipeline.h" "src/Render/Pipeline/Pipeline.cpp" "src/Render/Pipeline/GraphicsPipeline.h" "src/Render/Pipeline/GraphicsPipeline.cpp" "src/Render/Buffer/ModelManager.h" "src/Render/Buffer/ModelManager.cpp" "src/Render/Descriptors/DescriptorPool.h" "src/Render/Descriptors/DescriptorPool.cpp" "src/Render/Descriptors/DescriptorSetManager.h" "src/Render/Descriptors/DescriptorSetManager.cpp" "src/BluRendererVulkan.h" "include/RenderConst.h" "src/Render/Mesh/Mesh.cpp" "src/Render/Mesh/Mesh.h" "src/Engine/Scene/Scene.cpp" "src/Render/Camera/Camera.h" "src/Render/Camera/Camera.cpp"  "src/Render/Math/MathUtils.h" "src/Render/Math/MathUtils.cpp" "src/Render/Descriptors/Types/PushConsts/PushConst.h" "src/Engine/EngineCore/EngineCore.h" "src/Engine/EngineCore/EngineCore.cpp" "src/Engine/Input/Input.h" "src/Engine/Input/Input.cpp" "src/Render/RenderPass/RenderPassUtils.h" "src/Render/RenderPass/RenderPassUtils.cpp" "src/Render/Descriptors/DescriptorUtils.h" "src/Render/Descriptors/DescriptorUtils.cpp" "src/Render/Buffer/BufferManager.h" "src/Render/Buffer/BufferManager.cpp" "src/Render/Buffer/MappedBufferManager.h" "src/Render/Buffer/MappedBufferManager.cpp"  "src/Render/Image/ImageUtils.h" "src/Render/Image/ImageUtils.cpp" "src/Engine/Scene/SceneUtils.h" "src/Engine/Scene/SceneUtils.cpp")

target_include_directories(BluRendererVulkan PRIVATE source_dir, include_dir)
target_include_directories(BluRendererVulkan PRIVATE ${Vulkan_INCLUDE_DIRS})
target_link_libraries(BluRendererVulkan PUBLIC ${Vulkan_LIBRARIES} glfw)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET BluRendererVulkan PROPERTY CXX_STANDARD 20)
endif()