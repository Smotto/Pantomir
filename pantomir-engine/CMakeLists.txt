# --------------------------------------------------------------------
# Sources
# --------------------------------------------------------------------
# Only compile .cpp files; headers are discovered via includes
file(GLOB_RECURSE ENGINE_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp"
)

# Add headers for IDE visibility only
file(GLOB_RECURSE ENGINE_HEADERS
        "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/*.hpp"
)

# Executable
add_executable(pantomir-engine
        ${ENGINE_SOURCES}
        ${ENGINE_HEADERS}
)

# --------------------------------------------------------------------
# Include paths: public vs private
# --------------------------------------------------------------------
# Public headers exported to consumers (the rest of the project)
# Private headers are internal to this target
target_include_directories(pantomir-engine
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/source
)

# If your Dependencies.cmake sets Vulkan_INCLUDE_DIRS (or similar),
# keep it PRIVATE. It’s an implementation detail, not public API.
if (DEFINED Vulkan_INCLUDE_DIRS)
    target_include_directories(pantomir-engine PRIVATE ${Vulkan_INCLUDE_DIRS})
endif ()

# --------------------------------------------------------------------
# Defines
# --------------------------------------------------------------------
target_compile_definitions(pantomir-engine PRIVATE
        GLM_ENABLE_EXPERIMENTAL
        GLM_FORCE_DEPTH_ZERO_TO_ONE
)

# --------------------------------------------------------------------
# Subdirs / Libraries
# --------------------------------------------------------------------
add_subdirectory(engine-utils)

target_link_libraries(pantomir-engine PRIVATE
        engine-utils

        Vulkan::Vulkan
        glm::glm
        Stb::Stb
        fastgltf::fastgltf
        xxHash::xxhash
        vk-bootstrap::vk-bootstrap
        GPUOpen::VulkanMemoryAllocator
        SDL3::SDL3
        imgui::imgui
)

# --------------------------------------------------------------------
# Runtime output dir (matches root settings)
# --------------------------------------------------------------------
set_target_properties(pantomir-engine PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# --------------------------------------------------------------------
# Convenience "run" target via Terminal
# --------------------------------------------------------------------
add_custom_target(run-pantomir-engine
        COMMAND ${CMAKE_COMMAND} -E chdir "${CMAKE_SOURCE_DIR}" "$<TARGET_FILE:pantomir-engine>"
        DEPENDS pantomir-engine
        USES_TERMINAL
)
