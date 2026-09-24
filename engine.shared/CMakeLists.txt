# define project
project(engine.shared C CXX)
SET(TARGET_NAME engine.shared)

# configure executable, properties and linkage
add_library(${TARGET_NAME} STATIC)

# add sources to target
target_sources(
    ${TARGET_NAME}
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/appinfo.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/namespaces.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/vertex.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/enginetime.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/mesh.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/log.h"


        "${CMAKE_CURRENT_SOURCE_DIR}/math/vec2.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/math/vec2int.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/math/vec3.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/math/mat4x4.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/math/mathutils.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/math/quaternion.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/math/types.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/math/fixed.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/math/vec3fixed.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/math/mat4x4fixed.h"

        "${CMAKE_CURRENT_SOURCE_DIR}/utils/color.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/utils/util.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/utils/exceptions.h"

        "${CMAKE_CURRENT_SOURCE_DIR}/containers/fast_mutex.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/containers/tinythread.h"

        
        # "${CMAKE_CURRENT_SOURCE_DIR}/utils/singleton.h"

        "${CMAKE_CURRENT_SOURCE_DIR}/simd/cpufeatures.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/simd/transformbatch.h"

    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/enginetime.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/mesh.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/log.cpp"

        "${CMAKE_CURRENT_SOURCE_DIR}/utils/color.cpp"

        "${CMAKE_CURRENT_SOURCE_DIR}/containers/tinythread.cpp"

        "${CMAKE_CURRENT_SOURCE_DIR}/simd/cpufeatures.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/simd/transformbatch_baseline.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/simd/transformbatch_avx2.cpp"
)
# set additional target properties
set_target_properties(${TARGET_NAME} PROPERTIES LINKER_LANGUAGE CXX DEBUG_POSTFIX ".debug")

# engine.shared's own headers reference each other by root-relative path (e.g.
# utils/exceptions.h includes "namespaces.h", not "../namespaces.h"), so its own root needs
# to be on the include path - both for compiling engine.shared itself and for consumers,
# most of which already add this manually today.
target_include_directories(${TARGET_NAME} PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_SOURCE_DIR}/engine.glm"
)

# PUBLIC: log.h is included from engine.shared's own public header (log.h itself), so every
# consumer needs spdlog's headers on its include path too, same as engine.glm above.
target_link_libraries(${TARGET_NAME} PUBLIC
    spdlog::spdlog
)

# GLM_FORCE_DEPTH_ZERO_TO_ONE: D3D/Vulkan both use a [0,1] NDC depth range, not OpenGL's
# default [-1,1] - verified empirically (see conv_check throwaway test from the migration)
# that this exactly reproduces the old hand-rolled Mat4x4::FromPerspectiveFOV's output.
# GLM_FORCE_RADIANS: this project always passes/expects radians at the glm call boundary.
# PUBLIC so every consumer's translation units agree (mixing these across TUs that both
# touch the same glm types is an ODR violation).
target_compile_definitions(${TARGET_NAME} PUBLIC
    GLM_FORCE_DEPTH_ZERO_TO_ONE
    GLM_FORCE_RADIANS
    GLM_FORCE_INTRINSICS
)

# transformbatch_avx2.cpp is the one translation unit allowed to emit AVX2/FMA instructions -
# everything else in the binary stays on the safe SSE2 baseline (guaranteed present on any
# x86-64 CPU) so the binary doesn't crash with an illegal instruction on older hardware.
# CpuFeatures::Detect() checks AVX2/FMA support (including the OS XSAVE/YMM check, not just
# the CPUID bit) at runtime before this code path is ever called - see simd/cpufeatures.h.
if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    set_source_files_properties(
        "${CMAKE_CURRENT_SOURCE_DIR}/simd/transformbatch_avx2.cpp"
        PROPERTIES COMPILE_OPTIONS "/arch:AVX2"
    )
else()
    set_source_files_properties(
        "${CMAKE_CURRENT_SOURCE_DIR}/simd/transformbatch_avx2.cpp"
        PROPERTIES COMPILE_OPTIONS "-mavx2;-mfma"
    )
endif()