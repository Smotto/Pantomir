# === ImGui ===
find_package(imgui CONFIG QUIET)

if(TARGET imgui::imgui)
    message(STATUS "Found ImGui via CMake config")
    # No need to configure anything else since imgui::imgui is already set up
else()
    message(STATUS "ImGui config not found. Falling back to FetchContent...")

    include(FetchContent)
    FetchContent_Declare(
            imgui
            GIT_REPOSITORY https://github.com/ocornut/imgui.git
            GIT_TAG v1.91.8
    )

    FetchContent_GetProperties(imgui)
    if(NOT imgui_POPULATED)
        FetchContent_Populate(imgui)

        # Create ImGui library target
        add_library(imgui STATIC
                ${imgui_SOURCE_DIR}/imgui.cpp
                ${imgui_SOURCE_DIR}/imgui_draw.cpp
                ${imgui_SOURCE_DIR}/imgui_widgets.cpp
                ${imgui_SOURCE_DIR}/imgui_tables.cpp
                # ${imgui_SOURCE_DIR}/imgui_demo.cpp  # Uncomment if you want demo
        )

        target_include_directories(imgui PUBLIC
                $<BUILD_INTERFACE:${imgui_SOURCE_DIR}>
                $<INSTALL_INTERFACE:include>
        )

        # Find and link dependencies for Vulkan backend
        find_package(glfw3 CONFIG REQUIRED)
        find_package(Vulkan REQUIRED)

        target_sources(imgui PRIVATE
                ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
                ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
        )

        target_include_directories(imgui PUBLIC
                $<BUILD_INTERFACE:${imgui_SOURCE_DIR}/backends>
        )

        target_link_libraries(imgui PUBLIC
                glfw
                Vulkan::Vulkan
        )

        target_compile_features(imgui PUBLIC cxx_std_11)

        # Alias to match expected target name
        add_library(imgui::imgui ALIAS imgui)

        message(STATUS "ImGui configured successfully from source with Vulkan and GLFW backends")
    else()
        message(FATAL_ERROR "Failed to populate ImGui from FetchContent")
    endif()
endif()

# Organize in IDE - Check the actual target name
if(TARGET imgui)  # For when we build it ourselves
    set_property(TARGET imgui PROPERTY FOLDER "ExternalDependencies/ImGui")
elseif(TARGET imgui::imgui)  # For when found via config
    # We can't set properties on imported targets directly, but we can try to get the underlying target
    get_target_property(IMGUI_REAL_TARGET imgui::imgui ALIASED_TARGET)
    if(IMGUI_REAL_TARGET)
        set_property(TARGET ${IMGUI_REAL_TARGET} PROPERTY FOLDER "ExternalDependencies/ImGui")
    endif()
endif()