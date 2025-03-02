cmake_minimum_required(VERSION 3.31)
project(PantomirPlatform LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Collect platform sources and headers
file(GLOB_RECURSE PLATFORM_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
if(NOT PLATFORM_SOURCES)
    message(FATAL_ERROR "No platform source files found in ${CMAKE_CURRENT_SOURCE_DIR}/src")
endif()

# Define the static library
add_library(PantomirPlatform STATIC ${PLATFORM_SOURCES})

# Add headers as private sources (optional, for IDE visibility)
file(GLOB_RECURSE PLATFORM_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp" "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")
target_sources(PantomirPlatform PRIVATE ${PLATFORM_HEADERS})

# Public include directory for platform consumers
target_include_directories(PantomirPlatform PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Dependencies
target_link_libraries(PantomirPlatform PRIVATE
    glfw  # Windowing and input
    glm   # Math library
)

# Optional: Set output directory properties (if not set globally)
# set_target_properties(PantomirPlatform PROPERTIES
#     ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
# )

# Documentation
# This library provides platform abstraction (e.g., windowing, input) for the PantomirEngine,
# leveraging GLFW for cross-platform support and GLM for math utilities.