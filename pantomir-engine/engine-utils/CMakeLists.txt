# engine-utils — static utility library for Pantomir engine

file(GLOB_RECURSE UTIL_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp")
if(NOT UTIL_SOURCES)
    message(FATAL_ERROR "No source files found in ${CMAKE_CURRENT_SOURCE_DIR}/source/")
endif()

add_library(engine-utils STATIC ${UTIL_SOURCES})

target_include_directories(engine-utils PUBLIC include)

# Headers listed for IDE visibility only
file(GLOB_RECURSE UTIL_HEADERS
        "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
)
target_sources(engine-utils PRIVATE ${UTIL_HEADERS})