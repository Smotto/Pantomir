# === ImGui ===
# Built from source with SDL3 + Vulkan backends.
# Expects SDL3::SDL3 and Vulkan::Vulkan to already be available
# (resolved by Dependencies.cmake before this file is included).

include(FetchContent)
FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG v1.91.8
)

FetchContent_MakeAvailable(imgui)
FetchContent_GetProperties(imgui)

# Core library
add_library(imgui STATIC
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_demo.cpp
)

target_include_directories(imgui PUBLIC
        $<BUILD_INTERFACE:${imgui_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
)

# SDL3 + Vulkan backends
target_sources(imgui PRIVATE
        ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
)

target_include_directories(imgui PUBLIC
        $<BUILD_INTERFACE:${imgui_SOURCE_DIR}/backends>
)

target_link_libraries(imgui PUBLIC
        SDL3::SDL3
        Vulkan::Vulkan
)

target_compile_features(imgui PUBLIC cxx_std_11)

add_library(imgui::imgui ALIAS imgui)

# Organize in IDE
set_property(TARGET imgui PROPERTY FOLDER "ExternalDependencies/ImGui")