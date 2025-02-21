cmake_minimum_required(VERSION 3.14)
project(PantomirEditor LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Optionally set compiler-specific flags
if (MSVC)
    add_compile_options(/W4)
endif()

# Collect editor source files
file(GLOB_RECURSE EDITOR_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
if(NOT EDITOR_SOURCES)
    message(FATAL_ERROR "No editor source files found in ${CMAKE_CURRENT_SOURCE_DIR}/src")
endif()

add_executable(PantomirEditor ${EDITOR_SOURCES})

# Include editor headers
file(GLOB_RECURSE EDITOR_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp" "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")
target_sources(PantomirEditor PRIVATE ${EDITOR_HEADERS})
target_include_directories(PantomirEditor PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Link the engine library to the editor
target_link_libraries(PantomirEditor PUBLIC PantomirEngine)