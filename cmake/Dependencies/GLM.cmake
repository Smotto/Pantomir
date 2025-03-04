# === GLM ===
find_package(glm CONFIG QUIET)

if(TARGET glm::glm)
    message(STATUS "Found GLM via vcpkg")
else()
    message(WARNING "GLM not found in vcpkg. Falling back to FetchContent...")

    include(FetchContent)
    FetchContent_Declare(
            glm
            GIT_REPOSITORY https://github.com/g-truc/glm.git
            GIT_TAG 1.0.1
    )

    FetchContent_MakeAvailable(glm)

    if(TARGET glm::glm)
        message(STATUS "GLM built successfully from source.")
    else()
        message(FATAL_ERROR "Failed to fetch and build GLM from source.")
    endif()
endif()

# Organize in IDE
if(TARGET glm::glm)
    set_property(TARGET glm::glm PROPERTY FOLDER "ExternalDependencies/GLM")
endif()
