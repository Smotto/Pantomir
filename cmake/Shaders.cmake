set(SHADERS_SOURCE_DIR ${CMAKE_SOURCE_DIR}/Assets/Shaders)
set(SHADERS_BINARY_DIR ${SHADERS_SOURCE_DIR})  # Use source directory for compiled shaders
file(MAKE_DIRECTORY ${SHADERS_BINARY_DIR})

find_program(GLSLC glslc HINTS $ENV{VULKAN_SDK}/Bin)
find_program(GLSLANG_VALIDATOR glslangValidator HINTS $ENV{VULKAN_SDK}/Bin)
if (NOT GLSLC)
    message(FATAL_ERROR "glslc not found!")
endif()
if (NOT GLSLANG_VALIDATOR)
    message(WARNING "glslangValidator not found! Flame Graph support will be limited.")
endif()

file(GLOB SHADER_FILES CONFIGURE_DEPENDS "${SHADERS_SOURCE_DIR}/*.vert" "${SHADERS_SOURCE_DIR}/*.frag" "${SHADERS_SOURCE_DIR}/*.comp")

# Set shader compilation flags based on build type
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    if(GLSLANG_VALIDATOR)
        set(SHADER_COMPILER ${GLSLANG_VALIDATOR})
        set(SHADER_COMPILE_FLAGS -V -gVS)
        set(COMPILER_NAME "glslangValidator")
        message(STATUS "Compiling shaders with glslangValidator for full Flame Graph support")
    else()
        set(SHADER_COMPILER ${GLSLC})
        set(SHADER_COMPILE_FLAGS -g -O0)
        set(COMPILER_NAME "glslc")
        message(STATUS "Compiling shaders with glslc (limited Flame Graph support)")
    endif()
elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    if(GLSLANG_VALIDATOR)
        set(SHADER_COMPILER ${GLSLANG_VALIDATOR})
        set(SHADER_COMPILE_FLAGS -V -gVS)
        set(COMPILER_NAME "glslangValidator")
        message(STATUS "Compiling shaders with glslangValidator for full Flame Graph support")
    else()
        set(SHADER_COMPILER ${GLSLC})
        set(SHADER_COMPILE_FLAGS -g -O)
        set(COMPILER_NAME "glslc")
        message(STATUS "Compiling shaders with glslc (limited Flame Graph support)")
    endif()
else()
    set(SHADER_COMPILER ${GLSLC})
    set(SHADER_COMPILE_FLAGS -O)
    set(COMPILER_NAME "glslc")
    message(STATUS "Compiling shaders optimized (no debug symbols)")
endif()

foreach(shader ${SHADER_FILES})
    get_filename_component(shader_name ${shader} NAME)
    set(output_shader "${SHADERS_BINARY_DIR}/${shader_name}.spv")

    if(COMPILER_NAME STREQUAL "glslangValidator")
        add_custom_command(
                OUTPUT ${output_shader}
                COMMAND ${SHADER_COMPILER} ${SHADER_COMPILE_FLAGS} ${shader} -o ${output_shader}
                DEPENDS ${shader}
                COMMENT "Compiling ${shader_name} to SPIR-V with ${COMPILER_NAME} and flags: ${SHADER_COMPILE_FLAGS}"
        )
    else()
        add_custom_command(
                OUTPUT ${output_shader}
                COMMAND ${SHADER_COMPILER} ${SHADER_COMPILE_FLAGS} ${shader} -o ${output_shader}
                DEPENDS ${shader}
                COMMENT "Compiling ${shader_name} to SPIR-V with ${COMPILER_NAME} and flags: ${SHADER_COMPILE_FLAGS}"
        )
    endif()

    list(APPEND SHADER_BINARY_FILES ${output_shader})
endforeach()

add_custom_target(all_shaders ALL DEPENDS ${SHADER_BINARY_FILES})