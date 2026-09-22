# Compiles HLSL shaders to SPIR-V via dxc. This is the single build step for both backends:
# Vulkan consumes the .spirv output directly, D3D11/D3D12 cross-compile it back to HLSL at
# runtime via SPIRV-Cross (see driver.d3d/d3d11renderer.cpp CreateShader()).
#
# Replaces the old res/shadercompiler.bat, which was Windows-only and ran unconditionally at
# `cmake` configure time (via execute_process) instead of as a real, incremental build step.

find_program(DXC_EXECUTABLE dxc)
if(NOT DXC_EXECUTABLE)
    message(FATAL_ERROR "dxc (DirectXShaderCompiler) not found on PATH - required to compile res/**/*.hlsl to SPIR-V.")
endif()

set(SHADER_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/bin/data/shd")
file(MAKE_DIRECTORY "${SHADER_OUTPUT_DIR}")

set(SHADER_OUTPUTS "")

function(compile_hlsl_shader SRC PROFILE ENTRY)
    get_filename_component(SHADER_NAME "${SRC}" NAME_WE)
    set(OUT "${SHADER_OUTPUT_DIR}/${SHADER_NAME}.spirv")
    add_custom_command(
        OUTPUT "${OUT}"
        # -fvk-t-shift/-fvk-s-shift: without them, DXC assigns SPIR-V binding numbers
        # per HLSL register class independently, so t0/s0/b0 all land on binding 0 and
        # collide in a single Vulkan descriptor set (verified directly via spirv-cross
        # --reflect - this isn't visible from the D3D11 path at all, since D3D11 binds by
        # explicit slot index and never looks at these register numbers). Shifting t/s out
        # of the way of the untouched b0/b1 cbuffer bindings makes every resource's binding
        # number unique: cbuffers at 0-1, textures at 10+, samplers at 20+.
        COMMAND "${DXC_EXECUTABLE}" -spirv -T ${PROFILE} -E ${ENTRY} -fvk-t-shift 10 0 -fvk-s-shift 20 0 "${SRC}" -Fo "${OUT}"
        DEPENDS "${SRC}" "${CMAKE_CURRENT_SOURCE_DIR}/structures.hlsl"
        COMMENT "Compiling shader ${SHADER_NAME} (${PROFILE})"
        VERBATIM
    )
    set(SHADER_OUTPUTS "${SHADER_OUTPUTS};${OUT}" PARENT_SCOPE)
endfunction()

file(GLOB VS_SHADER_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/vs/*.hlsl")
foreach(SHADER ${VS_SHADER_SOURCES})
    compile_hlsl_shader("${SHADER}" vs_5_0 VS_Main)
endforeach()

file(GLOB PS_SHADER_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/ps/*.hlsl")
foreach(SHADER ${PS_SHADER_SOURCES})
    compile_hlsl_shader("${SHADER}" ps_5_0 PS_Main)
endforeach()

add_custom_target(shaders ALL DEPENDS ${SHADER_OUTPUTS})
