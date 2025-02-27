cmake_minimum_required(VERSION 3.31)
project(PantomirEditor LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (MSVC)
    add_compile_options(/W4)
endif()

file(GLOB_RECURSE EDITOR_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
if(NOT EDITOR_SOURCES)
    message(FATAL_ERROR "No editor source files found in ${CMAKE_CURRENT_SOURCE_DIR}/src")
endif()

add_executable(PantomirEditor ${EDITOR_SOURCES})

file(GLOB_RECURSE EDITOR_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp" "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")
target_sources(PantomirEditor PRIVATE ${EDITOR_HEADERS})
target_include_directories(PantomirEditor PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(PantomirEditor PUBLIC PantomirEngine)

if(MSVC)
    set_target_properties(PantomirEditor PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    )
endif()

set_target_properties(PantomirEditor PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/PantomirEditor"
)