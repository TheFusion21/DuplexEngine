# Build glue for the Dear ImGui submodule vendored at engine.editor/imgui - lives here rather
# than in engine.editor's own CMakeLists.txt because both driver.vulkan and driver.d3d (not
# just engine.editor itself) need to link against it: each renderer backend implements
# Renderer::InitImGui/ImGuiNewFrame/ImGuiRenderDrawData internally (see driver.graphics/
# renderer.h), so the ImGui backend sources need to be available at that layer, not just at
# the editor executable. This directory is deliberately not the submodule itself (a vendored
# third-party checkout shouldn't carry project-specific build files) - it just references it by
# relative path.
project(engine.imgui C CXX)
SET(TARGET_NAME engine.imgui)
SET(IMGUI_ROOT "${CMAKE_SOURCE_DIR}/engine.editor/imgui")

add_library(${TARGET_NAME} STATIC)

target_sources(
    ${TARGET_NAME}
    PUBLIC
        "${IMGUI_ROOT}/imgui.h"
        "${IMGUI_ROOT}/imconfig.h"
        "${IMGUI_ROOT}/imgui_internal.h"
        "${IMGUI_ROOT}/backends/imgui_impl_sdl.h"
        "${IMGUI_ROOT}/backends/imgui_impl_vulkan.h"

    PRIVATE
        "${IMGUI_ROOT}/imgui.cpp"
        "${IMGUI_ROOT}/imgui_draw.cpp"
        "${IMGUI_ROOT}/imgui_widgets.cpp"
        "${IMGUI_ROOT}/imgui_demo.cpp"
        "${IMGUI_ROOT}/backends/imgui_impl_sdl.cpp"
        "${IMGUI_ROOT}/backends/imgui_impl_vulkan.cpp"
)

set_target_properties(${TARGET_NAME} PROPERTIES LINKER_LANGUAGE CXX DEBUG_POSTFIX ".debug")

target_include_directories(${TARGET_NAME} PUBLIC
    "${IMGUI_ROOT}"
    "${IMGUI_ROOT}/backends"
)

target_link_libraries(${TARGET_NAME} PUBLIC
    Vulkan::Vulkan
    SDL2::SDL2
)

# D3D11 is Windows-only; the DX11 backend source only builds there.
if(WIN32)
    target_sources(${TARGET_NAME}
        PUBLIC "${IMGUI_ROOT}/backends/imgui_impl_dx11.h"
        PRIVATE "${IMGUI_ROOT}/backends/imgui_impl_dx11.cpp"
    )
    target_link_libraries(${TARGET_NAME} PUBLIC d3d11)
endif()
