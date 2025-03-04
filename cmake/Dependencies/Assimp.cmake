# === Assimp ===
find_package(assimp CONFIG QUIET)

if(TARGET assimp::assimp)
    message(STATUS "Found Assimp via vcpkg")
else()
    message(WARNING "Assimp not found in vcpkg. Falling back to FetchContent...")

    include(FetchContent)
    FetchContent_Declare(
            assimp
            GIT_REPOSITORY https://github.com/assimp/assimp.git
            GIT_TAG v5.4.3  # Use a stable version
            SOURCE_DIR ${CMAKE_BINARY_DIR}/_deps/assimp-src
            BINARY_DIR ${CMAKE_BINARY_DIR}/_deps/assimp-build
    )

    # Set Assimp build options
    set(ASSIMP_BUILD_SHARED_LIBS OFF CACHE BOOL "Build Assimp as static library" FORCE)
    set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "Disable Assimp tests" FORCE)
    set(ASSIMP_INSTALL OFF CACHE BOOL "Disable Assimp install" FORCE)

    FetchContent_MakeAvailable(assimp)

    if(TARGET assimp::assimp)
        message(STATUS "Assimp built successfully from source.")
    else()
        message(FATAL_ERROR "Failed to fetch and build Assimp from source.")
    endif()
endif()

# Organize in IDE
set_property(TARGET assimp::assimp PROPERTY FOLDER "ExternalDependencies/assimp")
