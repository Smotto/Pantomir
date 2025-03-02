cmake_minimum_required(VERSION 3.31)
project(PantomirEditor LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler options
if (MSVC)
    add_compile_options(/W4)
endif()

# Collect editor sources and headers
file(GLOB_RECURSE EDITOR_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
if(NOT EDITOR_SOURCES)
    message(FATAL_ERROR "No editor source files found in ${CMAKE_CURRENT_SOURCE_DIR}/src")
endif()

# Define the executable
add_executable(PantomirEditor ${EDITOR_SOURCES})

# Add headers as private sources (optional, for IDE visibility)
file(GLOB_RECURSE EDITOR_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp" "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")
target_sources(PantomirEditor PRIVATE ${EDITOR_HEADERS})

# Public include directory for editor-specific headers
target_include_directories(PantomirEditor PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Link to PantomirEngine
target_link_libraries(PantomirEditor PRIVATE PantomirEngine)

# MSVC-specific settings
if(MSVC)
    set_target_properties(PantomirEditor PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    )
endif()

# Output directory (optional, if not set globally)
set_target_properties(PantomirEditor PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Documentation
# This executable is the Pantomir Editor, a graphical interface for interacting with the PantomirEngine.