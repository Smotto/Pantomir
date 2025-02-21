cmake_minimum_required(VERSION 3.14)
project(PantomirRendererVulkan LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Collect Vulkan renderer sources and headers
file(GLOB_RECURSE RENDERER_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
if(NOT RENDERER_SOURCES)
   message(FATAL_ERROR "No renderer source files found in ${CMAKE_CURRENT_SOURCE_DIR}/src")
endif()

add_library(PantomirRendererVulkan STATIC ${RENDERER_SOURCES})

file(GLOB_RECURSE RENDERER_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp" "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")
target_sources(PantomirRendererVulkan PRIVATE ${RENDERER_HEADERS})

# Add the local include directory where your Vulkan headers reside
target_include_directories(PantomirRendererVulkan PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${Vulkan_INCLUDE_DIRS}
    ${glfw_SOURCE_DIR}/include
)

# Vulkan dependency
find_package(Vulkan REQUIRED)
target_link_libraries(PantomirRendererVulkan PRIVATE Vulkan::Vulkan)

# Link external dependencies for the renderer
target_link_libraries(PantomirRendererVulkan PRIVATE glfw glm tinyobjloader stb)

# Compiler definitions specific to Vulkan rendering
target_compile_definitions(PantomirRendererVulkan PRIVATE
    NOMINMAX
    VK_USE_PLATFORM_WIN32_KHR
    GLFW_INCLUDE_VULKAN
    GLFW_EXPOSE_NATIVE_WIN32

    GLM_FORCE_RADIANS
    GLM_FORCE_DEPTH_ZERO_TO_ONE

    STB_IMAGE_IMPLEMENTATION
    TINYOBJLOADER_IMPLEMENTATION
)
