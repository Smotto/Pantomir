cmake_minimum_required(VERSION 3.31)
project(Pantomir LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

include(cmake/Dependencies.cmake)

add_subdirectory(pantomir-engine)
include(cmake/Shaders.cmake)
add_dependencies(pantomir-engine all_shaders)
add_custom_target(ExtraSources SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/README.md)
add_dependencies(ExtraSources all_shaders)
