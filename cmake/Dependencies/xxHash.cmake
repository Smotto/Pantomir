# === xxHash ===
# Fast hash function library
find_package(xxHash CONFIG QUIET)

if(TARGET xxHash::xxhash)
    message(STATUS "Found xxHash via vcpkg")
else()
    message(WARNING "xxHash not found in vcpkg. Falling back to FetchContent...")

    include(FetchContent)
    FetchContent_Declare(
            xxhash
            GIT_REPOSITORY https://github.com/Cyan4973/xxHash.git
            GIT_TAG v0.8.3
            SOURCE_DIR ${CMAKE_BINARY_DIR}/_deps/xxhash-src
            BINARY_DIR ${CMAKE_BINARY_DIR}/_deps/xxhash-build
    )

    FetchContent_MakeAvailable(xxhash)

    if(NOT TARGET xxhash)
        add_library(xxhash INTERFACE)
        target_include_directories(xxhash INTERFACE ${xxhash_SOURCE_DIR})
        message(STATUS "Successfully built xxHash from source.")
    else()
        message(FATAL_ERROR "Failed to fetch and build xxHash from source.")
    endif()
endif()

# Organize in IDE
if(TARGET xxhash)
    set_property(TARGET xxhash PROPERTY FOLDER "ExternalDependencies/xxHash")
endif()
