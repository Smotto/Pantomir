if(NOT DEFINED ENV{VCPKG_ROOT})
    message(FATAL_ERROR "VCPKG_ROOT environment variable is not set.")
endif()

# This might not be needed, but it gets rid of a warning.
set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "vcpkg toolchain file")

include(${CMAKE_CURRENT_LIST_DIR}/Dependencies/Assimp.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/Dependencies/GLFW.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/Dependencies/STB.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/Dependencies/GLM.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/Dependencies/xxHash.cmake)