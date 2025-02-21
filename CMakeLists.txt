cmake_minimum_required(VERSION 3.14)
project(Pantomir)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# Include external dependencies
include(cmake/Dependencies.cmake)

# Subprojects
add_subdirectory(PantomirEngine)
add_subdirectory(PantomirEditor)

# A custom target for extra sources (just for convenience)
add_custom_target(ExtraSources SOURCES ${CMAKE_SOURCE_DIR}/README.md)

# === Copy Textures and Models ===
# Ensure the output directories exist
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Assets/Textures)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Assets/Models)

# Attach copy commands to ExtraSources
add_custom_command(TARGET ExtraSources POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          ${CMAKE_CURRENT_SOURCE_DIR}/Assets/Textures
          ${CMAKE_CURRENT_BINARY_DIR}/Assets/Textures
  COMMENT "Copying textures directory..."
)

add_custom_command(TARGET ExtraSources POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          ${CMAKE_CURRENT_SOURCE_DIR}/Assets/Models
          ${CMAKE_CURRENT_BINARY_DIR}/Assets/Models
  COMMENT "Copying models directory..."
)

# === Compile Shaders with glslc ===
set(SHADERS_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Assets/Shaders)
set(SHADERS_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/Assets/Shaders)
file(MAKE_DIRECTORY ${SHADERS_BINARY_DIR})

find_program(GLSLC glslc HINTS $ENV{VULKAN_SDK}/Bin)
if(NOT GLSLC)
  message(FATAL_ERROR "glslc not found! Please ensure the Vulkan SDK is installed and the VULKAN_SDK environment variable is set.")
endif()

file(GLOB SHADER_FILES RELATIVE ${SHADERS_SOURCE_DIR}
     "${SHADERS_SOURCE_DIR}/*.vert"
     "${SHADERS_SOURCE_DIR}/*.frag")

if(NOT SHADER_FILES)
  message(WARNING "No shader files found in ${SHADERS_SOURCE_DIR}")
endif()

set(SHADER_BINARY_FILES "")
foreach(shader ${SHADER_FILES})
  set(output_shader "${SHADERS_BINARY_DIR}/${shader}.spv")
  add_custom_command(
    OUTPUT ${output_shader}
    COMMAND ${GLSLC} ${SHADERS_SOURCE_DIR}/${shader} -o ${output_shader}
    DEPENDS ${SHADERS_SOURCE_DIR}/${shader}
    COMMENT "Compiling ${shader} to SPIR-V"
  )
  list(APPEND SHADER_BINARY_FILES ${output_shader})
endforeach()

add_custom_target(all_shaders ALL DEPENDS ${SHADER_BINARY_FILES})
add_dependencies(ExtraSources all_shaders)

# Ensure ExtraSources is built when building the main project
add_dependencies(PantomirEditor ExtraSources)