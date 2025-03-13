cmake_minimum_required(VERSION 3.31)
project(pantomir-engine LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Collect engine sources and headers
file(GLOB_RECURSE ENGINE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/source/*.hpp" "${CMAKE_CURRENT_SOURCE_DIR}/source/*.h")

# Define the executable instead of a library
add_executable(pantomir-engine ${ENGINE_SOURCES})

# Add headers as private sources (optional, for IDE visibility)
file(GLOB_RECURSE ENGINE_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp" "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")
target_sources(pantomir-engine PRIVATE ${ENGINE_HEADERS})

# Vulkan
target_include_directories(pantomir-engine PRIVATE ${Vulkan_INCLUDE_DIRS})

# Public include directory for engine consumers
target_include_directories(pantomir-engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Platform-specific definitions
target_compile_definitions(pantomir-engine PRIVATE
        GLM_FORCE_DEPTH_ZERO_TO_ONE
)

# Utils
add_subdirectory(engine-utils)

# Link submodules as private dependencies (internal to the engine)
target_link_libraries(pantomir-engine PRIVATE
        engine-utils
)

target_link_libraries(pantomir-engine PRIVATE
        Vulkan::Vulkan       # Vulkan
        glm::glm             # Math library
        assimp::assimp       # Model loading
        Stb::Stb             # Image loading
        xxHash::xxhash       # Hashing models
        vk-bootstrap::vk-bootstrap # Custom Vulkan library for initialization
        GPUOpen::VulkanMemoryAllocator
        SDL3::SDL3           # Window
)

# Set output directory properties
set_target_properties(pantomir-engine PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)