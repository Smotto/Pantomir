# === Vulkan Memory Allocator ===
find_package(VulkanMemoryAllocator CONFIG QUIET)

if(TARGET GPUOpen::VulkanMemoryAllocator)
    message(STATUS "Found Vulkan Memory Allocator via vcpkg")
else()
    message(WARNING "Vulkan Memory Allocator not found in vcpkg. Falling back to FetchContent...")

    include(FetchContent)
    FetchContent_Declare(
            vma
            GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
            GIT_TAG v3.0.1  # Use a stable version
            SOURCE_DIR ${CMAKE_BINARY_DIR}/_deps/vma-src
    )

    # Set any VMA build options if needed before making it available
    # VMA is header-only, so there aren't many build options

    # Use VMA's own CMake configuration
    FetchContent_MakeAvailable(vma)

    if(TARGET GPUOpen::VulkanMemoryAllocator)
        message(STATUS "Vulkan Memory Allocator built successfully from source.")
    else()
        message(FATAL_ERROR "Failed to fetch and build Vulkan Memory Allocator from source.")
    endif()
endif()

# Organize in IDE if the target exists directly
# Note: GPUOpen::VulkanMemoryAllocator is likely an ALIAS target, so we need to find the actual target
if(TARGET VulkanMemoryAllocator)
    set_property(TARGET VulkanMemoryAllocator PROPERTY FOLDER "ExternalDependencies/VulkanMemoryAllocator")
endif()