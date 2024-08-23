project(shaders)
cmake_minimum_required(VERSION 3.16)

set(OUTPUT_DIR .)

set(VK_VERSION "vulkan1.2")
# As of version 1.2.141.0, DXC is included with the Vulkan SDK.
# Change this variable if you want to use a local dxc executable.
set(DXC_COMPILER "dxc")

set(SHADERS_CS "initial_spectrum.cs.hlsl" "phase.cs.hlsl" "spectrum.cs.hlsl" "normal_map.cs.hlsl" "fft_horizontal.cs.hlsl" "fft_vertical.cs.hlsl")
set(SHADERS_DS)
set(SHADERS_PS "imgui.ps.hlsl" "ocean.ps.hlsl" "blit.ps.hlsl")
set(SHADERS_VS "imgui.vs.hlsl" "ocean.vs.hlsl" "blit.vs.hlsl")
set(SHADERS_GS)
set(SHADERS_HS)
set(SHADERS_LIB)

SET(SPIRV_BINARY_FILES)
function(Generate_Shaders SHADERS_SRC_LIST SHADER_TYPE)
    foreach (Shader IN LISTS ${SHADERS_SRC_LIST})
        get_filename_component(FILE_NAME ${Shader} NAME_WLE)
        message(STATUS "Calculating dependencies for shader ${SHADER_TYPE} ${FILE_NAME}")
        if (${SHADER_TYPE} STREQUAL vs OR ${SHADER_TYPE} STREQUAL ds OR ${SHADER_TYPE} STREQUAL gs)
            set(INVERT_Y TRUE)
        endif()

        set(SPIRV_OUTPUT "./${OUTPUT_DIR}/${FILE_NAME}.spv")
        set(SPIRV_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/${Shader}")
        set(COMMAND_PARAMS ${DXC_COMPILER}        # DXC compiler
            "${SPIRV_SOURCE}"                     # Source to be compiled
           -T "${SHADER_TYPE}_6_5"                # Shader profile
           -I "${CMAKE_CURRENT_SOURCE_DIR}/../"   # Include directory
           -spirv                                 # Generate SPIR-V for Vulkan
           -Fo ${SPIRV_OUTPUT}                    # Output file
           #$<$<BOOL:${INVERT_Y}>:-fvk-invert-y>
           -fvk-use-gl-layout                     # Memory layout
           -fspv-target-env=${VK_VERSION}         # Target environment
           -enable-16bit-types                    # Enable fp16
        )

        # Determine include dependencies (recursively) This works by compiling
        # the shader with the option -Vi, which seems to print all inclusion
        # operations done during compilation.  The process is quite slow, it
        # would be nice to have a dedicated option in dxc to do this properly,
        # like the -MM option in gcc/clang.
        execute_process(COMMAND ${COMMAND_PARAMS} -Vi
            ERROR_VARIABLE INCLUDE_DEPENDENCIES
            OUTPUT_VARIABLE CMD_OUT
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_STRIP_TRAILING_WHITESPACE
        )

        if (INCLUDE_DEPENDENCIES)
            string(REPLACE "\n" ";" INCLUDE_DEPENDENCIES ${INCLUDE_DEPENDENCIES}) # transform lines in cmake list
            list(FILTER INCLUDE_DEPENDENCIES INCLUDE REGEX "Opening file") # remove non dependencies lines
            list(LENGTH INCLUDE_DEPENDENCIES INCLUDE_DEPENDENCIES_LENGTH)
            if(${INCLUDE_DEPENDENCIES_LENGTH} GREATER 0)
                # filter out [inplace] filename from program output
                list(TRANSFORM INCLUDE_DEPENDENCIES REPLACE "Opening file \\[(.+)\\], stack top.*" "\\1")
            endif()
        endif()

        # DEBUG PRINT
        foreach(ELEM ${INCLUDE_DEPENDENCIES})
            message(${ELEM})
        endforeach()

        add_custom_command(
            OUTPUT ${SPIRV_OUTPUT}
            COMMAND ${CMAKE_COMMAND} -E make_directory "${OUTPUT_DIR}"
            COMMAND echo ${COMMAND_PARAMS}
            COMMAND ${COMMAND_PARAMS}
            DEPENDS ${SPIRV_SOURCE}
            DEPENDS ${INCLUDE_DEPENDENCIES}
        )
        list(APPEND SPIRV_BINARY_FILES ${SPIRV_OUTPUT})
    endforeach()
    set(SPIRV_BINARY_FILES ${SPIRV_BINARY_FILES} PARENT_SCOPE)
endfunction()

Generate_Shaders(SHADERS_VS   vs)
Generate_Shaders(SHADERS_HS   hs)
Generate_Shaders(SHADERS_DS   ds)
Generate_Shaders(SHADERS_GS   gs)
Generate_Shaders(SHADERS_PS   ps)
Generate_Shaders(SHADERS_CS   cs)
Generate_Shaders(SHADERS_LIB lib)

add_custom_target(Shaders_SPIRV DEPENDS ${SPIRV_BINARY_FILES})
