cmake_minimum_required(VERSION 3.31)

# ============================================================================
# Auto-detect vcpkg if VCPKG_ROOT is not set
# ============================================================================
if(NOT DEFINED ENV{VCPKG_ROOT})
    foreach(_vs_edition Community Professional Enterprise)
        set(_candidate "C:/Program Files/Microsoft Visual Studio/2022/${_vs_edition}/VC/vcpkg")
        if(EXISTS "${_candidate}/scripts/buildsystems/vcpkg.cmake")
            set(ENV{VCPKG_ROOT} "${_candidate}")
            message(STATUS "Auto-detected VCPKG_ROOT: ${_candidate}")
            break()
        endif()
    endforeach()

    if(NOT DEFINED ENV{VCPKG_ROOT})
        message(FATAL_ERROR
            "Could not find vcpkg. Either:\n"
            "  1. Set the VCPKG_ROOT environment variable, or\n"
            "  2. Install Visual Studio 2022 with 'Desktop development with C++'"
        )
    endif()
endif()

# Set toolchain before project() so vcpkg integrates properly
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "vcpkg toolchain file")
endif()

# ============================================================================
# Project
# ============================================================================
project(Pantomir LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# ============================================================================
# Dependencies, engine, shaders
# ============================================================================
include(cmake/Dependencies.cmake)

add_subdirectory(pantomir-engine)
include(cmake/Shaders.cmake)
add_dependencies(pantomir-engine all_shaders)
