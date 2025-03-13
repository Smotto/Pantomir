# === SDL3 ===
# Windowing and input library
find_package(SDL3 CONFIG QUIET)

if(TARGET SDL3::SDL3)
    message(STATUS "Found SDL3 via vcpkg")
else()
    message(WARNING "SDL3 not found in vcpkg. Falling back to FetchContent...")

    include(FetchContent)
    FetchContent_Declare(
            SDL3
            GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
            GIT_TAG release-3.2.8
    )

    # SDL3 specific options
    set(SDL_SHARED OFF CACHE BOOL "Build a SDL shared library" FORCE)
    set(SDL_STATIC ON CACHE BOOL "Build a SDL static library" FORCE)
    set(SDL_TEST OFF CACHE BOOL "Build the SDL test framework" FORCE)

    # Make sure Vulkan support is enabled in SDL3
    set(SDL_VULKAN ON CACHE BOOL "Enable Vulkan support in SDL3" FORCE)

    FetchContent_MakeAvailable(SDL3)

    if(TARGET SDL3::SDL3-static OR TARGET SDL3::SDL3)
        message(STATUS "SDL3 built successfully from source.")
    else()
        message(FATAL_ERROR "Failed to fetch and build SDL3 from source.")
    endif()
endif()

# Organize in IDE - avoid setting properties on ALIAS targets
if(TARGET SDL3-static)
    set_property(TARGET SDL3-static PROPERTY FOLDER "ExternalDependencies/SDL3")
elseif(TARGET SDL3)
    set_property(TARGET SDL3 PROPERTY FOLDER "ExternalDependencies/SDL3")
endif()