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

        "${CMAKE_CURRENT_SOURCE_DIR}/graphics/bsdfmaterial.h"

        "${CMAKE_CURRENT_SOURCE_DIR}/graphics/light.h"

    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/input/input.cpp"

        "${CMAKE_CURRENT_SOURCE_DIR}/graphics/texture.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/graphics/texture2d.cpp"

        "${CMAKE_CURRENT_SOURCE_DIR}/graphics/light.cpp"
)

# set additional target properties
set_target_properties(${TARGET_NAME} PROPERTIES LINKER_LANGUAGE CXX DEBUG_POSTFIX ".debug")

target_link_libraries(${TARGET_NAME}
    driver.d3d
    driver.vulkan
)
target_include_directories(${TARGET_NAME} PUBLIC
	"${CMAKE_SOURCE_DIR}/driver.d3d"
	"${CMAKE_SOURCE_DIR}/driver.vulkan"
)