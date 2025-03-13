# === STB ===
find_package(Stb QUIET)  # Remove CONFIG to allow module mode fallback

if(Stb_FOUND)
    message(STATUS "Found STB via vcpkg")
    # STB is header-only, so just ensure the include directory is available
    if(NOT TARGET Stb::Stb)
        add_library(Stb::Stb INTERFACE IMPORTED)
        set_target_properties(Stb::Stb PROPERTIES
                INTERFACE_INCLUDE_DIRECTORIES "${Stb_INCLUDE_DIR}"
        )
    endif()
else()
    message(WARNING "STB not found in vcpkg. Falling back to FetchContent...")
    include(FetchContent)
    FetchContent_Declare(
            stb
            GIT_REPOSITORY https://github.com/nothings/stb.git
            GIT_TAG 5736b15
    )
    FetchContent_MakeAvailable(stb)

    if(NOT TARGET Stb::Stb)
        add_library(Stb::Stb INTERFACE)
        target_include_directories(Stb::Stb INTERFACE ${stb_SOURCE_DIR})
        message(STATUS "Successfully built STB from source.")
    else()
        message(FATAL_ERROR "Failed to fetch and configure STB from source.")
    endif()
endif()

# Organize in IDE
if(TARGET Stb::Stb)
    set_property(TARGET Stb::Stb PROPERTY FOLDER "ExternalDependencies/STB")
endif()