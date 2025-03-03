cmake_minimum_required(VERSION 3.31)
project(PantomirUtils LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE UTIL_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
if(NOT UTIL_SOURCES)
    message(FATAL_ERROR "No util source files found in ${CMAKE_CURRENT_SOURCE_DIR}/src")
endif()

add_library(PantomirUtils STATIC ${UTIL_SOURCES})

target_include_directories(PantomirUtils PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

file(GLOB_RECURSE UTIL_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp" "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")
target_sources(PantomirUtils PRIVATE ${UTIL_HEADERS})