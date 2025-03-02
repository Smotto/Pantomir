cmake_minimum_required(VERSION 3.31)
project(Pantomir LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Dependencies
include(cmake/Dependencies.cmake)

# Subprojects
add_subdirectory(PantomirEngine)
add_subdirectory(PantomirEditor)

# Shader compilation
include(cmake/Shaders.cmake)
add_dependencies(PantomirEditor all_shaders)

# Extra sources
add_custom_target(ExtraSources SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/README.md)
add_dependencies(ExtraSources all_shaders)