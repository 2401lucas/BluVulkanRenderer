# CMakeList.txt : CMake project for BluRendererVulkan, include source and define
# project specific logic here.
cmake_minimum_required(VERSION 3.11)

set(ENV{VULKAN_SDK} "C:/VulkanSDK/1.3.261.1")
find_package(Vulkan)
find_package(glfw3 CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(gli CONFIG REQUIRED)
find_package(ktx CONFIG REQUIRED)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)

file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.h" "src/*.hpp" "src/**/*.cpp" "src/**/*.h" "src/**/*.hpp" "src/***/*.cpp" "src/***/*.h" "src/***/*.hpp" )

# Add source to this project's executable.
add_executable (BluRendererVulkan ${SOURCES} ${include_files})

target_include_directories(BluRendererVulkan PRIVATE source_dir, include_dir)
target_include_directories(BluRendererVulkan PRIVATE ${Vulkan_INCLUDE_DIRS})
target_include_directories(BluRendererVulkan PRIVATE ${Ktx_LIBRARY})
target_link_libraries(BluRendererVulkan PUBLIC ${Vulkan_LIBRARIES} glfw)
target_link_libraries(BluRendererVulkan PRIVATE imgui::imgui)
target_link_libraries(BluRendererVulkan PRIVATE gli)
target_link_libraries(BluRendererVulkan PUBLIC ${Ktx_LIBRARY} KTX::ktx)
target_link_libraries(BluRendererVulkan PRIVATE Vulkan::Headers GPUOpen::VulkanMemoryAllocator)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET BluRendererVulkan PROPERTY CXX_STANDARD 20)
endif()