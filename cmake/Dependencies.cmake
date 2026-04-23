# ============================================================================
# Dependencies.cmake — Resolve all third-party dependencies
# ============================================================================
# Strategy:
#   1. vcpkg packages are resolved via find_package in each module below.
#   2. If vcpkg doesn't provide a package, the module falls back to FetchContent.
#   3. Vulkan SDK is a system dependency (must be installed on the OS).
#
# NOTE: VCPKG_ROOT auto-detection and CMAKE_TOOLCHAIN_FILE are handled
#       in the root CMakeLists.txt before project().
# ============================================================================

# ----------------------------------------------------------------------------
# System dependency
# ----------------------------------------------------------------------------
find_package(Vulkan REQUIRED)

# ----------------------------------------------------------------------------
# vcpkg-only (no FetchContent fallback needed)
# ----------------------------------------------------------------------------
find_package(fastgltf CONFIG REQUIRED)

# ----------------------------------------------------------------------------
# Per-library modules (vcpkg-first, FetchContent fallback)
# Each module is self-contained: find_package + fallback + IDE folder.
# ----------------------------------------------------------------------------
include(${CMAKE_CURRENT_LIST_DIR}/Dependencies/SDL3.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/Dependencies/imgui.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/Dependencies/Stb.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/Dependencies/glm.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/Dependencies/xxHash.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/Dependencies/VulkanMemoryAllocator.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/Dependencies/vk-bootstrap.cmake)

# ----------------------------------------------------------------------------
# Validation — ensure critical FetchContent targets resolved
# ----------------------------------------------------------------------------
if(NOT TARGET vk-bootstrap AND NOT TARGET vk-bootstrap::vk-bootstrap)
    message(FATAL_ERROR "vk-bootstrap target not available. Check Dependencies/vk-bootstrap.cmake.")
endif()