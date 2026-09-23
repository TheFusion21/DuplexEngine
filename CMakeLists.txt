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
