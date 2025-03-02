set(SHADERS_SOURCE_DIR ${CMAKE_SOURCE_DIR}/Assets/Shaders)
set(SHADERS_BINARY_DIR ${SHADERS_SOURCE_DIR})  # Use source directory for compiled shaders
file(MAKE_DIRECTORY ${SHADERS_BINARY_DIR})

find_program(GLSLC glslc HINTS $ENV{VULKAN_SDK}/Bin)
if (NOT GLSLC)
    message(FATAL_ERROR "glslc not found!")
endif()

file(GLOB SHADER_FILES CONFIGURE_DEPENDS "${SHADERS_SOURCE_DIR}/*.vert" "${SHADERS_SOURCE_DIR}/*.frag")

foreach(shader ${SHADER_FILES})
    get_filename_component(shader_name ${shader} NAME)
    set(output_shader "${SHADERS_BINARY_DIR}/${shader_name}.spv")

    add_custom_command(
            OUTPUT ${output_shader}
            COMMAND ${GLSLC} ${shader} -o ${output_shader}
            DEPENDS ${shader}
            COMMENT "Compiling ${shader_name} to SPIR-V"
    )

    list(APPEND SHADER_BINARY_FILES ${output_shader})
endforeach()

add_custom_target(all_shaders ALL DEPENDS ${SHADER_BINARY_FILES})
