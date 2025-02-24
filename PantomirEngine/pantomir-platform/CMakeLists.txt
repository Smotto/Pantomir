cmake_minimum_required(VERSION 3.31)
project(PantomirPlatform LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

### 1) Collect Source and Header Files ###
file(GLOB_RECURSE PLATFORM_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
if(NOT PLATFORM_SOURCES)
    message(FATAL_ERROR "No platform source files found in ${CMAKE_CURRENT_SOURCE_DIR}/src")
endif()
add_library(${PROJECT_NAME} STATIC ${PLATFORM_SOURCES})

file(GLOB_RECURSE PLATFORM_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
)
target_sources(${PROJECT_NAME} PRIVATE ${PLATFORM_HEADERS})

### 2) GLFW ###
target_link_libraries(${PROJECT_NAME} PRIVATE glfw)
target_include_directories(${PROJECT_NAME} PUBLIC ${glfw_SOURCE_DIR}/include)

### 3) Platform's Own Include Directory ###
target_include_directories(${PROJECT_NAME} PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)