cmake_minimum_required(VERSION 3.31)
project(PantomirRendererVulkan LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Collect Vulkan renderer sources and headers
file(GLOB_RECURSE RENDERER_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
if(NOT RENDERER_SOURCES)
    message(FATAL_ERROR "No renderer source files found in ${CMAKE_CURRENT_SOURCE_DIR}/src")
endif()

add_library(PantomirRendererVulkan STATIC ${RENDERER_SOURCES})

file(GLOB_RECURSE RENDERER_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp" "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")
target_sources(PantomirRendererVulkan PRIVATE ${RENDERER_HEADERS})

# Include directories
target_include_directories(PantomirRendererVulkan PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${Vulkan_INCLUDE_DIRS}
    ${glfw_SOURCE_DIR}/include
)

# Link dependencies
find_package(Vulkan REQUIRED)
target_link_libraries(PantomirRendererVulkan PRIVATE
    Vulkan::Vulkan
    glfw
    glm
    tinyobjloader
    stb
)

# Dependency on PantomirPlatform (PUBLIC if users need it, PRIVATE if internal)
target_link_libraries(PantomirRendererVulkan PUBLIC PantomirPlatform) # Keep PUBLIC for now

# Compiler definitions
target_compile_definitions(PantomirRendererVulkan PRIVATE
    NOMINMAX
    GLFW_INCLUDE_VULKAN
    GLFW_EXPOSE_NATIVE_WIN32
    GLM_FORCE_RADIANS
    GLM_FORCE_DEPTH_ZERO_TO_ONE
)