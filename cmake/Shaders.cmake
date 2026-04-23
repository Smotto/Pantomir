# ============================================================================
# Shaders.cmake — Compile GLSL shaders to SPIR-V
# ============================================================================
# Supports Ninja Multi-Config by generating a per-config custom command
# that selects the correct compiler and flags.
#
# Debug / RelWithDebInfo:
#   - Prefers glslangValidator (full flame-graph debug symbols via -gVS)
#   - Falls back to glslc with -g if glslangValidator is unavailable
# Release:
#   - Always uses glslc with -O for optimized output
#
# Output: .spv files alongside sources in Assets/Shaders/
#         (gitignored via *.spv)
# ============================================================================

set(SHADERS_SOURCE_DIR ${CMAKE_SOURCE_DIR}/Assets/Shaders)
set(SHADERS_OUTPUT_DIR ${SHADERS_SOURCE_DIR})
file(MAKE_DIRECTORY ${SHADERS_OUTPUT_DIR})

# ----------------------------------------------------------------------------
# Locate shader compilers
# ----------------------------------------------------------------------------
find_program(GLSLC glslc HINTS $ENV{VULKAN_SDK}/Bin)
find_program(GLSLANG_VALIDATOR glslangValidator HINTS $ENV{VULKAN_SDK}/Bin)

if(NOT GLSLC)
    message(FATAL_ERROR "glslc not found! Install the Vulkan SDK and ensure it is in PATH.")
endif()

if(NOT GLSLANG_VALIDATOR)
    message(WARNING
        "glslangValidator not found. Debug builds will use glslc "
        "(limited flame-graph support)."
    )
endif()

# ----------------------------------------------------------------------------
# Gather shader sources
# ----------------------------------------------------------------------------
file(GLOB SHADER_FILES CONFIGURE_DEPENDS
        "${SHADERS_SOURCE_DIR}/*.vert"
        "${SHADERS_SOURCE_DIR}/*.frag"
        "${SHADERS_SOURCE_DIR}/*.comp"
)

# ----------------------------------------------------------------------------
# Helper: create a CMake script that picks compiler+flags at build time
# This avoids generator-expression issues with Ninja Multi-Config.
# ----------------------------------------------------------------------------
set(SHADER_COMPILE_SCRIPT "${CMAKE_BINARY_DIR}/compile_shader.cmake")
if(GLSLANG_VALIDATOR)
    file(WRITE ${SHADER_COMPILE_SCRIPT} [=[
# compile_shader.cmake — invoked at build time
# Args: -DCONFIG=<config> -DSHADER_IN=<src> -DSHADER_OUT=<spv>
#       -DGLSLC=<path> -DGLSLANG=<path>
if(CONFIG STREQUAL "Debug" OR CONFIG STREQUAL "RelWithDebInfo")
    set(CMD ${GLSLANG} -V -gVS ${SHADER_IN} -o ${SHADER_OUT})
else()
    set(CMD ${GLSLC} -O ${SHADER_IN} -o ${SHADER_OUT})
endif()
execute_process(COMMAND ${CMD} RESULT_VARIABLE rc)
if(NOT rc EQUAL 0)
    message(FATAL_ERROR "Shader compilation failed: ${CMD}")
endif()
]=])
else()
    file(WRITE ${SHADER_COMPILE_SCRIPT} [=[
# compile_shader.cmake — invoked at build time (glslc only)
# Args: -DCONFIG=<config> -DSHADER_IN=<src> -DSHADER_OUT=<spv> -DGLSLC=<path>
if(CONFIG STREQUAL "Debug")
    set(CMD ${GLSLC} -g -O0 ${SHADER_IN} -o ${SHADER_OUT})
elseif(CONFIG STREQUAL "RelWithDebInfo")
    set(CMD ${GLSLC} -g -O ${SHADER_IN} -o ${SHADER_OUT})
else()
    set(CMD ${GLSLC} -O ${SHADER_IN} -o ${SHADER_OUT})
endif()
execute_process(COMMAND ${CMD} RESULT_VARIABLE rc)
if(NOT rc EQUAL 0)
    message(FATAL_ERROR "Shader compilation failed: ${CMD}")
endif()
]=])
endif()

# ----------------------------------------------------------------------------
# Compile each shader via the script (config resolved at build time)
# ----------------------------------------------------------------------------
foreach(shader ${SHADER_FILES})
    get_filename_component(shader_name ${shader} NAME)
    set(output_shader "${SHADERS_OUTPUT_DIR}/${shader_name}.spv")

    add_custom_command(
            OUTPUT ${output_shader}
            COMMAND ${CMAKE_COMMAND}
                -DCONFIG=$<CONFIG>
                -DSHADER_IN=${shader}
                -DSHADER_OUT=${output_shader}
                -DGLSLC=${GLSLC}
                -DGLSLANG=${GLSLANG_VALIDATOR}
                -P ${SHADER_COMPILE_SCRIPT}
            DEPENDS ${shader} ${SHADER_COMPILE_SCRIPT}
            COMMENT "Compiling ${shader_name} -> SPIR-V ($<CONFIG>)"
    )

    list(APPEND SHADER_BINARY_FILES ${output_shader})
endforeach()

add_custom_target(all_shaders ALL DEPENDS ${SHADER_BINARY_FILES})