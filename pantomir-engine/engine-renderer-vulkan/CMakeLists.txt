cmake_minimum_required(VERSION 3.31)
project(engine-renderer-vulkan LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Collect Vulkan renderer sources and headers
file(GLOB_RECURSE RENDERER_SOURCES "source/*.cpp")
if (NOT RENDERER_SOURCES)
    message(FATAL_ERROR "No renderer source files found in ${CMAKE_CURRENT_SOURCE_DIR}/source")
endif ()

# Define the static library
add_library(engine-renderer-vulkan STATIC ${RENDERER_SOURCES})

# Add headers as private sources (optional, for IDE visibility)
file(GLOB_RECURSE RENDERER_HEADERS "include/*.hpp" "include/*.h")
target_sources(engine-renderer-vulkan PRIVATE ${RENDERER_HEADERS})

# Public include directory for renderer consumers
target_include_directories(engine-renderer-vulkan PUBLIC
        include
)

# Vulkan setup
find_package(Vulkan REQUIRED)
target_link_libraries(engine-renderer-vulkan PRIVATE Vulkan::Vulkan)

# Engine's own submodule dependencies
target_link_libraries(engine-renderer-vulkan PRIVATE
        engine-platform
        engine-utils
)

# Dependencies
target_link_libraries(engine-renderer-vulkan PRIVATE
        glfw            # Windowing and input
        glm::glm             # Math library
        assimp::assimp          # Model loading (replacing tinyobjloader)
        Stb::Stb             # Image loading
        xxHash::xxhash          # Hashing models
)

# Compiler definitions
target_compile_definitions(engine-renderer-vulkan PRIVATE
        NOMINMAX                # Avoid Windows min/max macro conflicts
        GLFW_INCLUDE_VULKAN     # Include Vulkan headers with GLFW
        GLFW_EXPOSE_NATIVE_WIN32 # Enable native Win32 access
        GLM_FORCE_RADIANS        # Use radians for GLM
        GLM_FORCE_DEPTH_ZERO_TO_ONE # Vulkan depth range
)

# Documentation
# This library provides Vulkan-based rendering functionality for the pantomir-engine,
# integrating with pantomir-platform and supporting model loading via Assimp.