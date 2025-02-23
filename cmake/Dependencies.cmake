# cmake/Dependencies.cmake
# Minimum required version for using FetchContent
cmake_minimum_required(VERSION 3.31)

include(FetchContent)

# === TinyObjLoader ===
FetchContent_Declare(
    tinyobjloader
    GIT_REPOSITORY https://github.com/tinyobjloader/tinyobjloader.git
    GIT_TAG        release
)
FetchContent_MakeAvailable(tinyobjloader)
if(TARGET tinyobjloader)
    set_property(TARGET tinyobjloader PROPERTY FOLDER "ExternalDependencies/TinyObjLoader")
endif()

# === stb (header-only) ===
FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG        master
)
FetchContent_MakeAvailable(stb)
# Create an INTERFACE target for stb if it wasn't defined by FetchContent
if(NOT TARGET stb)
    add_library(stb INTERFACE)
    target_include_directories(stb INTERFACE ${stb_SOURCE_DIR})
endif()
set_property(TARGET stb PROPERTY FOLDER "ExternalDependencies/stb")

# === GLM (header-only) ===
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        1.0.1
)
FetchContent_MakeAvailable(glm)
if(TARGET glm::glm)
    get_target_property(glm_real glm::glm ALIASED_TARGET)
    if(glm_real)
        set_property(TARGET ${glm_real} PROPERTY FOLDER "ExternalDependencies/glm")
    endif()
endif()

# === GLFW ===
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.4
)
FetchContent_MakeAvailable(glfw)
if(TARGET glfw)
    set_property(TARGET glfw PROPERTY FOLDER "ExternalDependencies/GLFW")
endif()

set_target_properties(glfw PROPERTIES FOLDER "ExternalDependencies/GLFW3")
set_target_properties(update_mappings PROPERTIES FOLDER "ExternalDependencies/GLFW3")
set_target_properties(tinyobjloader PROPERTIES FOLDER "ExternalDependencies/TinyObjLoader")
set_target_properties(uninstall PROPERTIES FOLDER "ExternalDependencies/TinyObjLoader")