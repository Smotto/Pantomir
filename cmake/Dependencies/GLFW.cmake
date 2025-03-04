# === GLFW ===
# Windowing and input library
find_package(glfw3 CONFIG QUIET)

if(TARGET glfw)
    message(STATUS "Found GLFW via vcpkg")
else()
    message(WARNING "GLFW not found in vcpkg. Falling back to FetchContent...")

    include(FetchContent)
    FetchContent_Declare(
            glfw
            GIT_REPOSITORY https://github.com/glfw/glfw.git
            GIT_TAG 3.4
    )

    FetchContent_MakeAvailable(glfw)

    if(TARGET glfw)
        message(STATUS "GLFW built successfully from source.")
    else()
        message(FATAL_ERROR "Failed to fetch and build GLFW from source.")
    endif()
endif()

# Organize in IDE
set_property(TARGET glfw PROPERTY FOLDER "ExternalDependencies/GLFW")
