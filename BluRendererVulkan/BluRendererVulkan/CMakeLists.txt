# CMakeList.txt : CMake project for BluRendererVulkan, include source and define
# project specific logic here.
cmake_minimum_required(VERSION 3.13)

find_package(Vulkan REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)

# Add include directory manually (vcpkg should locate it automatically)
find_path(EASTL_INCLUDE_DIRS "EASTL/vector.h" HINTS "${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/include")

# Find the library manually
find_library(EASTL_LIB eastl HINTS "${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/lib")

file(GLOB_RECURSE SOURCES "*.cpp" "*.h")

add_subdirectory ("src")
add_subdirectory ("includes")
# Add source to this project's executable.
add_executable (BluRendererVulkan ${SOURCES} ${include_files})

set(EASTL_LIBRARY debug ${EASTL_ROOT_DIR}/build/Debug/EASTL.lib optimized ${EASTL_ROOT_DIR}/build/Release/EASTL.lib)

target_include_directories(BluRendererVulkan PRIVATE source_dir, include_dir)
  target_include_directories(BluRendererVulkan PRIVATE ${Vulkan_INCLUDE_DIRS})
  target_include_directories(BluRendererVulkan PRIVATE ${EASTL_INCLUDE_DIRS})
  target_link_libraries(BluRendererVulkan PRIVATE ${EASTL_LIB})
  target_link_libraries(BluRendererVulkan PUBLIC ${Vulkan_LIBRARIES} glfw)
  target_link_libraries(BluRendererVulkan PRIVATE Vulkan::Vulkan)
  target_link_libraries(BluRendererVulkan PRIVATE imgui::imgui)
  target_link_libraries(BluRendererVulkan PRIVATE Vulkan::Vulkan GPUOpen::VulkanMemoryAllocator)
  target_link_libraries(BluRendererVulkan PRIVATE nlohmann_json::nlohmann_json)
  target_link_libraries(BluRendererVulkan PRIVATE assimp::assimp)

  target_compile_definitions(BluRendererVulkan PUBLIC GLM_FORCE_DEPTH_ZERO_TO_ONE)
  target_compile_definitions(BluRendererVulkan PUBLIC GLM_ENABLE_EXPERIMENTAL)
  # target_compile_definitions(BluRendererVulkan PUBLIC DEBUG_UV)
  target_compile_definitions(BluRendererVulkan PUBLIC DEBUG_MODEL)
  target_compile_definitions(BluRendererVulkan PUBLIC DEBUG_LABELS)
  
if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET BluRendererVulkan PROPERTY CXX_STANDARD 20)
endif()