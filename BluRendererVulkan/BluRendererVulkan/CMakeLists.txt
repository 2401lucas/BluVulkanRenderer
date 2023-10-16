# CMakeList.txt : CMake project for BluRendererVulkan, include source and define
# project specific logic here.
cmake_minimum_required(VERSION 3.11)

find_package(Vulkan)
find_package(glfw3 CONFIG REQUIRED)

# Add source to this project's executable.
add_executable (BluRendererVulkan ${src_files} ${include_files} "src/BluRendererVulkan.cpp" "include/BluRendererVulkan.h" "src/FileManager.cpp" "include/FileManager.h" "src/RenderManager.cpp" "include/RenderManager.h" "src/Window/WindowManager.cpp" "src/Window/WindowManager.h" "src/VkInstance/VulkanInstance.cpp" "src/VkInstance/VulkanInstance.h" "src/Scene/Scene.h" "src/Model/Model.h" "src/Model/Model.cpp" "src/Image/Image.h" "src/Image/Image.cpp" "src/Device/Device.h" "src/Device/Device.cpp" "include/Settings/VkLayerConfig.h" "src/Swapchain/Swapchain.h" "src/Swapchain/Swapchain.cpp" "src/Renderer/RenderManager.cpp" "src/Renderer/RenderManager.h")

target_include_directories(BluRendererVulkan PRIVATE source_dir, include_dir)
target_include_directories(BluRendererVulkan PRIVATE ${Vulkan_INCLUDE_DIRS})
target_link_libraries(BluRendererVulkan PUBLIC ${Vulkan_LIBRARIES} glfw)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET BluRendererVulkan PROPERTY CXX_STANDARD 20)
endif()