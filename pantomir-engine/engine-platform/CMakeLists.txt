cmake_minimum_required(VERSION 3.31)
project(engine-platform LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Collect platform sources and headers
file(GLOB_RECURSE PLATFORM_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp")
if (NOT PLATFORM_SOURCES)
    message(FATAL_ERROR "No platform source files found in ${CMAKE_CURRENT_SOURCE_DIR}/source")
endif ()

# Define the static library
add_library(engine-platform STATIC ${PLATFORM_SOURCES})

# Add headers as private sources (optional, for IDE visibility)
file(GLOB_RECURSE PLATFORM_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp" "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")
target_sources(engine-platform PRIVATE ${PLATFORM_HEADERS})

# Public include directory for platform consumers
target_include_directories(engine-platform PUBLIC
        include
)

# Dependencies
target_link_libraries(engine-platform PRIVATE
        glfw  # Windowing and input
        imgui::imgui
        glm::glm   # Math library
        Vulkan::Vulkan
)

target_compile_definitions(engine-platform PRIVATE
        GLFW_INCLUDE_VULKAN     # Include Vulkan headers with GLFW
        GLFW_EXPOSE_NATIVE_WIN32 # Enable native Win32 access
)
