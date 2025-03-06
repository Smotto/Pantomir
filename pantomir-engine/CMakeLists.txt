cmake_minimum_required(VERSION 3.31)
project(pantomir-engine LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Collect engine sources and headers
file(GLOB_RECURSE ENGINE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp")
if (NOT ENGINE_SOURCES)
    message(FATAL_ERROR "No engine source files found in ${CMAKE_CURRENT_SOURCE_DIR}/source")
endif()

# Define the executable instead of a library
add_executable(pantomir-engine ${ENGINE_SOURCES})

# Add headers as private sources (optional, for IDE visibility)
file(GLOB_RECURSE ENGINE_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp" "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")
target_sources(pantomir-engine PRIVATE ${ENGINE_HEADERS})

# Public include directory for engine consumers
target_include_directories(pantomir-engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Vulkan setup
find_package(Vulkan REQUIRED)
target_include_directories(pantomir-engine PUBLIC ${Vulkan_INCLUDE_DIRS})
target_link_libraries(pantomir-engine PRIVATE Vulkan::Vulkan)

# Platform-specific definitions
target_compile_definitions(pantomir-engine PRIVATE
        VK_USE_PLATFORM_WIN32_KHR
)

# Link submodules
add_subdirectory(engine-platform)
add_subdirectory(engine-renderer-vulkan)
add_subdirectory(engine-utils)

# Link submodules as private dependencies (internal to the engine)
target_link_libraries(pantomir-engine PRIVATE
        engine-platform
        engine-renderer-vulkan
        engine-utils
)

# Optional: Enable editor mode (toggleable with --editor flag)
option(ENABLE_EDITOR "Include editor functionality" OFF)

if(ENABLE_EDITOR)
    find_package(imgui CONFIG REQUIRED)
    target_link_libraries(pantomir-engine PRIVATE imgui::imgui)
    target_compile_definitions(pantomir-engine PRIVATE ENABLE_EDITOR)
    # Add editor-specific sources if needed
    file(GLOB_RECURSE EDITOR_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/source/editor/*.cpp")
    target_sources(pantomir-engine PRIVATE ${EDITOR_SOURCES})
endif()

# Set output directory properties
set_target_properties(pantomir-engine PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Documentation
# This executable provides the core pantomir-engine functionality, integrating platform, renderer, and utils submodules.
# It can run in game mode by default, with optional editor mode enabled via ENABLE_EDITOR and --editor flag.