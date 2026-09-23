# define project
project(engine.core C CXX)
SET(TARGET_NAME engine.core)

# configure executable, properties and linkage
add_library(${TARGET_NAME} STATIC)
# add sources to target
target_sources(
    ${TARGET_NAME}
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/input/input.h"

        "${CMAKE_CURRENT_SOURCE_DIR}/graphics/stb_image.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/graphics/texture.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/graphics/texture2d.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/graphics/cubemap.h"

        "${CMAKE_CURRENT_SOURCE_DIR}/graphics/bsdfmaterial.h"
        

        "${CMAKE_CURRENT_SOURCE_DIR}/graphics/light.h"

        "${CMAKE_CURRENT_SOURCE_DIR}/ecs/components/transform.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/ecs/components/camera.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/ecs/components/meshrenderer.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/ecs/components/rigidbody.h"

        "${CMAKE_CURRENT_SOURCE_DIR}/ecs/systems/camerasystem.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/ecs/systems/meshsystem.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/ecs/systems/lightsystem.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/ecs/systems/physicssystem.h"

        "${CMAKE_CURRENT_SOURCE_DIR}/physics/physicsworld.h"

    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/input/input.cpp"

        "${CMAKE_CURRENT_SOURCE_DIR}/graphics/texture.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/graphics/texture2d.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/graphics/cubemap.cpp"

        "${CMAKE_CURRENT_SOURCE_DIR}/graphics/light.cpp"

        "${CMAKE_CURRENT_SOURCE_DIR}/physics/physicsworld.cpp"
)

# set additional target properties
set_target_properties(${TARGET_NAME} PROPERTIES LINKER_LANGUAGE CXX DEBUG_POSTFIX ".debug")

target_link_libraries(${TARGET_NAME}
    driver.vulkan
    SDL2::SDL2
    Jolt
)
target_include_directories(${TARGET_NAME} PUBLIC
	"${CMAKE_SOURCE_DIR}/driver.vulkan"
	"${CMAKE_SOURCE_DIR}/engine.entt/single_include"
)

# D3D11/D3D12 are Windows-only backends; engine.core stays buildable on other
# platforms with Vulkan alone.
if(WIN32)
    target_link_libraries(${TARGET_NAME}
        driver.d3d
        driver.d3d12
    )
    target_include_directories(${TARGET_NAME} PUBLIC
        "${CMAKE_SOURCE_DIR}/driver.d3d"
        "${CMAKE_SOURCE_DIR}/driver.d3d12"
    )
endif()