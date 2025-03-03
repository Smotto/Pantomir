cmake_minimum_required(VERSION 3.31)
project(PantomirEngine LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Collect engine sources and headers
file(GLOB_RECURSE ENGINE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
if (NOT ENGINE_SOURCES)
    message(FATAL_ERROR "No engine source files found in ${CMAKE_CURRENT_SOURCE_DIR}/src")
endif ()

# Define the static library
add_library(PantomirEngine STATIC ${ENGINE_SOURCES})

# Add headers as private sources (optional, for IDE visibility)
file(GLOB_RECURSE ENGINE_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp" "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")
target_sources(PantomirEngine PRIVATE ${ENGINE_HEADERS})

# Public include directory for engine consumers
target_include_directories(PantomirEngine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Vulkan setup
find_package(Vulkan REQUIRED)
target_include_directories(PantomirEngine PUBLIC ${Vulkan_INCLUDE_DIRS})
target_link_libraries(PantomirEngine PRIVATE Vulkan::Vulkan)

# Platform-specific definitions
target_compile_definitions(PantomirEngine PRIVATE
        VK_USE_PLATFORM_WIN32_KHR
)

# Link submodules (assumed to be in Submodules/ directory)
add_subdirectory(Submodules/pantomir-platform)
add_subdirectory(Submodules/pantomir-renderer/vulkan-backend)
add_subdirectory(Submodules/pantomir-utils)

# Link submodules as private dependencies (internal to the engine)
target_link_libraries(PantomirEngine PRIVATE
        PantomirPlatform
        PantomirRendererVulkan
        PantomirUtils
)

# Optional: Set output directory properties (if not set globally in root)
# set_target_properties(PantomirEngine PROPERTIES
#     ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
# )

# Documentation
# This library provides the core engine functionality, integrating platform and renderer submodules.