if(NOT DEFINED ENV{VCPKG_ROOT})
    message(FATAL_ERROR "VCPKG_ROOT environment variable is not set.")
endif()

# This might not be needed, but it gets rid of a warning.
set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "vcpkg toolchain file")

# All files in the Dependencies folder need to be included.
include(${CMAKE_CURRENT_LIST_DIR}/Dependencies/imgui.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/Dependencies/assimp.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/Dependencies/SDL3.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/Dependencies/Stb.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/Dependencies/glm.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/Dependencies/xxHash.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/Dependencies/VulkanMemoryAllocator.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/Dependencies/vk-bootstrap.cmake)

find_package(Vulkan REQUIRED) # Vulkan INSTALLED ON YOUR OS

# All vcpkgs need to be found for CMake
find_package(glm CONFIG REQUIRED)
find_package(SDL3 CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)
find_package(Stb REQUIRED) # Might want to use just the image part for Stb
find_package(xxHash CONFIG REQUIRED)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)

# All packages fetched via Github
if(NOT TARGET vk-bootstrap AND NOT TARGET vk-bootstrap::vk-bootstrap)
    message(FATAL_ERROR "vk-bootstrap target not available. Check your Dependencies/vk-bootstrap.cmake setup.")
endif()