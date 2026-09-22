# define project
project(driver.d3d C CXX)
SET(TARGET_NAME driver.d3d)

# configure executable, properties and linkage
add_library(${TARGET_NAME} STATIC)

# add sources to target
target_sources(
    ${TARGET_NAME}
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/d3d11renderer.h"

    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/d3d11renderer.cpp"
)

# set additional target properties
set_target_properties(${TARGET_NAME} PROPERTIES LINKER_LANGUAGE CXX DEBUG_POSTFIX ".debug")

# d3d11.lib/dxgi.lib for the D3D11/DXGI API (<d3d11.h>/<dxgi.h>) and d3dcompiler.lib for
# D3DCompile (<d3dcompiler.h>) - previously missing, so anything actually linking driver.d3d
# failed with unresolved externals for D3D11CreateDeviceAndSwapChain/D3DCompile.
target_link_libraries(${TARGET_NAME}
    driver.graphics
    spirv-cross-hlsl
    d3d11.lib
    dxgi.lib
    d3dcompiler.lib
    SDL2::SDL2
)
target_include_directories(${TARGET_NAME} PUBLIC
	"${CMAKE_SOURCE_DIR}/driver.graphics"
	"${CMAKE_SOURCE_DIR}/engine.spirv"
)