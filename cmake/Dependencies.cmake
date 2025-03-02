cmake_minimum_required(VERSION 3.31)
include(FetchContent)

cmake_policy(SET CMP0175 OLD)  # Accept old behavior, silence warnings

# Ensure static linking globally
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries" FORCE)

# === Assimp ===
# Model loading library (replacing TinyObjLoader)
find_package(assimp QUIET)
if (NOT assimp_FOUND)
    message(STATUS "Assimp not found locally, fetching from source...")
    FetchContent_Declare(
        assimp
        GIT_REPOSITORY https://github.com/assimp/assimp.git
        GIT_TAG v5.4.3  # Specific stable version
        SOURCE_DIR ${CMAKE_BINARY_DIR}/_deps/assimp-src
        SUBBUILD_DIR ${CMAKE_BINARY_DIR}/_deps/assimp-subbuild
        BINARY_DIR ${CMAKE_BINARY_DIR}/_deps/assimp-build
    )
    # Explicitly disable shared libraries for Assimp
    set(ASSIMP_BUILD_SHARED_LIBS OFF CACHE BOOL "Build Assimp as shared library" FORCE)
    message(STATUS "Assimp BUILD_SHARED_LIBS: ${BUILD_SHARED_LIBS}")
    message(STATUS "Assimp ASSIMP_BUILD_SHARED_LIBS: ${ASSIMP_BUILD_SHARED_LIBS}")
    FetchContent_MakeAvailable(assimp)
endif()
if(TARGET assimp)
    set_property(TARGET assimp PROPERTY FOLDER "ExternalDependencies/assimp")
endif()
set_target_properties(zlibstatic PROPERTIES FOLDER "ExternalDependencies/assimp")
set_target_properties(unit PROPERTIES FOLDER "ExternalDependencies/assimp")

# === TinyObjLoader ===
# Legacy model loader (optional, included for compatibility)
message(STATUS "Fetching TinyObjLoader with tag v1.0.7...")
FetchContent_Declare(
    tinyobjloader
    GIT_REPOSITORY https://github.com/tinyobjloader/tinyobjloader.git
    GIT_TAG 2945a96
    SOURCE_DIR ${CMAKE_BINARY_DIR}/_deps/tinyobjloader-src
    SUBBUILD_DIR ${CMAKE_BINARY_DIR}/_deps/tinyobjloader-subbuild
    BINARY_DIR ${CMAKE_BINARY_DIR}/_deps/tinyobjloader-build
)
FetchContent_MakeAvailable(tinyobjloader)
if(TARGET tinyobjloader)
    set_property(TARGET tinyobjloader PROPERTY FOLDER "ExternalDependencies/TinyObjLoader")
endif()
set_target_properties(uninstall PROPERTIES FOLDER "ExternalDependencies/TinyObjLoader")

# === STB ===
# Header-only image loading library
FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG 5736b15
)
FetchContent_MakeAvailable(stb)
if(NOT TARGET stb)
    add_library(stb INTERFACE)
    target_include_directories(stb INTERFACE ${stb_SOURCE_DIR})
endif()
set_property(TARGET stb PROPERTY FOLDER "ExternalDependencies/stb")

# === GLM ===
# Header-only math library
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.1
)
FetchContent_MakeAvailable(glm)
if(TARGET glm::glm)
    get_target_property(glm_real glm::glm ALIASED_TARGET)
    if(glm_real)
        set_property(TARGET ${glm_real} PROPERTY FOLDER "ExternalDependencies/glm")
    endif()
endif()

# === GLFW ===
# Windowing and input library
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
)
FetchContent_MakeAvailable(glfw)
if(TARGET glfw)
    set_property(TARGET glfw PROPERTY FOLDER "ExternalDependencies/GLFW")
endif()
set_target_properties(update_mappings PROPERTIES FOLDER "ExternalDependencies/GLFW")