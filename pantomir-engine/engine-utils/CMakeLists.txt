cmake_minimum_required(VERSION 3.31)
project(engine-utils LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE UTIL_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp")
if(NOT UTIL_SOURCES)
    message(FATAL_ERROR "No util source files found in ${CMAKE_CURRENT_SOURCE_DIR}/src")
endif()

add_library(engine-utils STATIC ${UTIL_SOURCES})

target_include_directories(engine-utils PUBLIC include)

file(GLOB_RECURSE UTIL_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp" "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")
target_sources(engine-utils PRIVATE ${UTIL_HEADERS})