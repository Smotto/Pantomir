# === ImGui ===
# Find SDL3 from vcpkg
find_package(SDL3 REQUIRED)

# Find Vulkan (system dependency)
find_package(Vulkan REQUIRED)

# Set up ImGui using FetchContent
include(FetchContent)
FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG v1.91.8
)

# Use the newer MakeAvailable approach instead of Populate
FetchContent_MakeAvailable(imgui)

# Get the source directory after making imgui available
FetchContent_GetProperties(imgui)

# Create ImGui library target
add_library(imgui STATIC
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_demo.cpp  # Uncomment if you want demo
)

target_include_directories(imgui PUBLIC
        $<BUILD_INTERFACE:${imgui_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
)

# Add SDL3 and Vulkan backends
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

# Alias to match expected target name
add_library(imgui::imgui ALIAS imgui)

message(STATUS "ImGui configured successfully from source with SDL3 and Vulkan backends")

# Organize in IDE
set_property(TARGET imgui PROPERTY FOLDER "ExternalDependencies/ImGui")