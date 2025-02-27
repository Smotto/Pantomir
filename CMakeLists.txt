cmake_minimum_required(VERSION 3.31)
project(Pantomir LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# Include external dependencies
include(cmake/Dependencies.cmake)

# Subprojects
add_subdirectory(PantomirEngine)
add_subdirectory(PantomirEditor)

# A custom target for extra sources (just for convenience)
add_custom_target(ExtraSources SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/README.md)

# === Compile Shaders with glslc ===
set(SHADERS_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Assets/Shaders)
set(SHADERS_BINARY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Assets/Shaders) # Or use ${CMAKE_CURRENT_BINARY_DIR}/Shaders
file(MAKE_DIRECTORY ${SHADERS_BINARY_DIR})

find_program(GLSLC glslc HINTS $ENV{VULKAN_SDK}/Bin)
if (NOT GLSLC)
    message(FATAL_ERROR "glslc not found! Please ensure the Vulkan SDK is installed and the VULKAN_SDK environment variable is set.")
endif ()

file(GLOB SHADER_FILES CONFIGURE_DEPENDS
        "${SHADERS_SOURCE_DIR}/*.vert"
        "${SHADERS_SOURCE_DIR}/*.frag")

if (NOT SHADER_FILES)
    message(WARNING "No shader files found in ${SHADERS_SOURCE_DIR}")
endif ()

set(SHADER_BINARY_FILES "")
foreach(shader ${SHADER_FILES})
    # Extract the base filename to avoid path duplication
    get_filename_component(shader_name ${shader} NAME)
    set(output_shader "${SHADERS_BINARY_DIR}/${shader_name}.spv")
    add_custom_command(
            OUTPUT ${output_shader}
            COMMAND ${GLSLC} ${SHADERS_SOURCE_DIR}/${shader_name} -o ${output_shader}
            DEPENDS ${SHADERS_SOURCE_DIR}/${shader_name}
            COMMENT "Compiling ${shader_name} to SPIR-V"
            VERBATIM
    )
    list(APPEND SHADER_BINARY_FILES ${output_shader})
endforeach()

add_custom_target(all_shaders ALL DEPENDS ${SHADER_BINARY_FILES})
add_dependencies(ExtraSources all_shaders)
add_dependencies(PantomirEditor all_shaders) # Ensure shaders are built before the editor