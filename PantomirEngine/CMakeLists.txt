cmake_minimum_required(VERSION 3.31)
project(PantomirEngine LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (MSVC)
    add_compile_options(/W4)
endif()

# Collect engine sources and headers
file(GLOB_RECURSE ENGINE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
if(NOT ENGINE_SOURCES)
    message(FATAL_ERROR "No engine source files found in ${CMAKE_CURRENT_SOURCE_DIR}/src")
endif()

add_library(PantomirEngine STATIC ${ENGINE_SOURCES})
# Optionally add headers as private sources
file(GLOB_RECURSE ENGINE_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp" "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")
target_sources(PantomirEngine PRIVATE ${ENGINE_HEADERS})

# Public include directory for engine headers
target_include_directories(PantomirEngine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

### VULKAN
find_package(Vulkan REQUIRED)
target_include_directories(${PROJECT_NAME} PUBLIC ${Vulkan_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PRIVATE Vulkan::Vulkan)

target_compile_definitions(PantomirEngine PRIVATE
    VK_USE_PLATFORM_WIN32_KHR
)

# Link submodules: Platform and Renderer
add_subdirectory(pantomir-platform)
add_subdirectory(pantomir-renderer/vulkan-backend)
target_link_libraries(PantomirEngine PUBLIC PantomirPlatform PantomirRendererVulkan)