# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.8)

project ("PR210")

include(compiler.cmake)
find_package(Vulkan REQUIRED)
find_package(SDL2 REQUIRED)

# Keep the SPIRV-Cross submodule build lean: we only need the library, not its own
# CLI/test suite.
set(SPIRV_CROSS_ENABLE_TESTS OFF CACHE BOOL "" FORCE)
set(SPIRV_CROSS_CLI OFF CACHE BOOL "" FORCE)
add_subdirectory ("engine.spirv")

# Phase 9: cross-platform-deterministic physics (see engine.jolt's own README for what this
# flag guarantees, and project memory for the research behind choosing it). Jolt's
# Build/CMakeLists.txt auto-disables its UnitTests/Samples/JoltViewer/HelloWorld/
# PerformanceTest targets whenever it's add_subdirectory'd from a parent project (it checks
# CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR), so only the Jolt library itself gets
# built here - no extra options needed for that part.
# USE_AVX/USE_AVX2 forced OFF (Jolt defaults both ON): this project's own SIMD story (see
# engine.shared/simd/) deliberately keeps the whole binary on an SSE2-safe baseline and only
# ever emits AVX2 in one runtime-dispatched translation unit - letting Jolt bake AVX2 into
# every translation unit it compiles would silently make the entire game binary crash with
# an illegal instruction on any CPU without AVX2, contradicting that. CROSS_PLATFORM_
# DETERMINISTIC is orthogonal to this (it's about getting the same numeric RESULT across
# platforms once running, not about which instructions the binary is allowed to contain), so
# both are compatible with each other and set independently.
set(CROSS_PLATFORM_DETERMINISTIC ON CACHE BOOL "" FORCE)
set(USE_AVX OFF CACHE BOOL "" FORCE)
set(USE_AVX2 OFF CACHE BOOL "" FORCE)
set(USE_AVX512 OFF CACHE BOOL "" FORCE)
# Jolt defaults INTERPROCEDURAL_OPTIMIZATION (LTO) ON, and its own CMakeLists.txt comments warn
# this can produce an "error adding symbols: file format not recognized" link error unless the
# consuming project also enables LTO for its own targets. GCC/ld on this dev box linked the
# resulting libJolt.a into the final executables without issue either way, but that's only
# verified for this one toolchain - MSVC's /LTCG has different mixed-LTO/non-LTO quirks this
# can't be verified against here (no Windows toolchain available). Disabling it costs
# essentially nothing (LTO mainly helps cross-translation-unit inlining, not correctness) and
# removes a whole category of risk on the untested platform.
set(INTERPROCEDURAL_OPTIMIZATION OFF CACHE BOOL "" FORCE)
# Jolt's Build/CMakeLists.txt defaults JPH_USE_DX12/JPH_USE_VK/JPH_USE_MTL/JPH_USE_CPU_COMPUTE
# all ON - these build an entirely separate, experimental GPU-compute module (hair simulation
# compute shaders under Jolt/Compute/, unrelated to Jolt's CPU rigid-body physics this project
# actually integrated in Phase 9) with its own HLSL->DXIL/SPIR-V shader compile step and its own
# find_package(Vulkan) call, independent of this project's own driver.vulkan. Never previously
# caught because Phase 9 was only ever verified on Linux, where JPH_USE_DX12's DXIL path is
# inert anyway. On Windows this breaks the build two different ways: a warning-as-error
# (NTDDI_WIN11_DT undefined) inside ComputeSystemVK*.cpp against the newer 10.0.26100.0 SDK, and
# a "file not found" from Jolt's own dxc invocation for the Hair*.hlsl shaders (relative -I path
# assumes a working directory MSBuild's custom-build step doesn't give it). It's also the root
# cause of the GitHub Actions Windows job failing at CMake's generate step with a bogus
# "VULKAN_SDK/include" path baked into Jolt's INTERFACE_INCLUDE_DIRECTORIES - Jolt's own
# find_package(Vulkan) resolves differently than this project's (line 9 above), depending on how
# the CI runner's Vulkan SDK install action sets $ENV{VULKAN_SDK}. None of this module is used
# by anything this engine does, so disabling it outright (matching the CACHE BOOL "" FORCE
# pattern already used above) removes the whole fragile surface rather than debugging it.
set(JPH_USE_DX12 OFF CACHE BOOL "" FORCE)
set(JPH_USE_VK OFF CACHE BOOL "" FORCE)
set(JPH_USE_MTL OFF CACHE BOOL "" FORCE)
set(JPH_USE_CPU_COMPUTE OFF CACHE BOOL "" FORCE)
add_subdirectory ("engine.jolt/Build")

# Cross-platform subprojects: buildable and tested on both Windows and Linux.
add_subdirectory ("driver.graphics")
# Needs to come before driver.vulkan (and, on Windows, driver.d3d below) - both link against it
# for their Renderer::InitImGui/etc implementations. See engine.imgui/CMakeLists.txt.
add_subdirectory ("engine.imgui")
add_subdirectory ("driver.vulkan")
add_subdirectory ("engine.shared")
add_subdirectory ("engine.core")
add_subdirectory ("engine.config")
add_subdirectory ("engine.window")
add_subdirectory ("engine.client")
add_subdirectory ("engine.editor")

# D3D11/D3D12 only exist on Windows.
if(WIN32)
    add_subdirectory ("driver.d3d")
    add_subdirectory ("driver.d3d12")
endif()

# Compiles res/vs/*.hlsl and res/ps/*.hlsl to SPIR-V via dxc, cross-platform, as a proper
# build-time target (replaces the old Windows-only shadercompiler.bat that ran unconditionally
# at configure time).
add_subdirectory ("res")

enable_testing()
add_subdirectory ("tests")
