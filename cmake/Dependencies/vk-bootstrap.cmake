# === vk-bootstrap ===
# Vulkan initialization library
find_package(vk-bootstrap CONFIG QUIET)

# Organize in IDE
if(TARGET vk-bootstrap::vk-bootstrap)
    set_property(TARGET vk-bootstrap::vk-bootstrap PROPERTY FOLDER "ExternalDependencies/vk-bootstrap")
else()
    include(FetchContent)
    FetchContent_Declare(
            vk-bootstrap
            GIT_REPOSITORY https://github.com/charles-lunarg/vk-bootstrap
            GIT_TAG v1.4.310
    )

    FetchContent_MakeAvailable(vk-bootstrap)

    # We don't need to find_package after building from source
    # The targets should be directly available
    if(TARGET vk-bootstrap)
        message(STATUS "vk-bootstrap built successfully from source.")
    else()
        message(FATAL_ERROR "Failed to fetch and build vk-bootstrap from source.")
    endif()
endif()