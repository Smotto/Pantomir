cmake_minimum_required(VERSION 3.31)
project(PantomirRendererVulkan LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Collect Vulkan renderer sources and headers
file(GLOB_RECURSE RENDERER_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
if (NOT RENDERER_SOURCES)
    message(FATAL_ERROR "No renderer source files found in ${CMAKE_CURRENT_SOURCE_DIR}/src")
endif ()

# Define the static library
add_library(PantomirRendererVulkan STATIC ${RENDERER_SOURCES})

# Add headers as private sources (optional, for IDE visibility)
file(GLOB_RECURSE RENDERER_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp" "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")
target_sources(PantomirRendererVulkan PRIVATE ${RENDERER_HEADERS})

# Public include directory for renderer consumers
target_include_directories(PantomirRendererVulkan PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Vulkan setup
find_package(Vulkan REQUIRED)
target_link_libraries(PantomirRendererVulkan PRIVATE Vulkan::Vulkan)

# Dependencies
target_link_libraries(PantomirRendererVulkan PRIVATE
        glfw            # Windowing and input
        glm             # Math library
        assimp          # Model loading (replacing tinyobjloader)
        tinyobjloader
        stb             # Image loading
        PantomirPlatform # Platform abstraction
)

# Compiler definitions
target_compile_definitions(PantomirRendererVulkan PRIVATE
        NOMINMAX                # Avoid Windows min/max macro conflicts
        GLFW_INCLUDE_VULKAN     # Include Vulkan headers with GLFW
        GLFW_EXPOSE_NATIVE_WIN32 # Enable native Win32 access
        GLM_FORCE_RADIANS        # Use radians for GLM
        GLM_FORCE_DEPTH_ZERO_TO_ONE # Vulkan depth range
)

# Optional: Set output directory properties (if not set globally)
# set_target_properties(PantomirRendererVulkan PROPERTIES
#     ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
# )

# Documentation
# This library provides Vulkan-based rendering functionality for the PantomirEngine,
# integrating with PantomirPlatform and supporting model loading via Assimp.