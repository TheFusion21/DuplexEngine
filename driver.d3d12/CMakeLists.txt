# define project
project(driver.d3d12 C CXX)
SET(TARGET_NAME driver.d3d12)

# configure executable, properties and linkage
add_library(${TARGET_NAME} STATIC)

# add sources to target
target_sources(
    ${TARGET_NAME}
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/d3d12renderer.h"

    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/d3d12renderer.cpp"
)

# set additional target properties
set_target_properties(${TARGET_NAME} PROPERTIES LINKER_LANGUAGE CXX DEBUG_POSTFIX ".debug")

# spirv-cross-hlsl: cross-compiles the res/CMakeLists.txt-produced SPIR-V back to HLSL at
# runtime (see d3d12renderer.cpp CreateShader()) - same runtime shader pipeline as driver.d3d,
# not a build-time DXIL/SM6 path. d3dcompiler.lib: D3DCompile, compiling that cross-compiled
# HLSL to SM5.0 bytecode a D3D12 PSO can consume directly. SDL2/engine.imgui: window-handle
# retrieval (SDL_GetWindowWMInfo) and Dear ImGui's D3D12 backend (Renderer::InitImGui).
target_link_libraries(${TARGET_NAME}
    driver.graphics
    spirv-cross-hlsl
    d3d12.lib
    dxgi.lib
    d3dcompiler.lib
    SDL2::SDL2
    engine.imgui
)
target_include_directories(${TARGET_NAME} PUBLIC
	"${CMAKE_SOURCE_DIR}/driver.graphics"
	"${CMAKE_SOURCE_DIR}/engine.spirv"
)
