# define project
project(engine.shared.math C CXX)
SET(TARGET_NAME engine.shared.math)

# configure executable, properties and linkage
add_library(${TARGET_NAME} STATIC)

# add sources to target
target_sources(
    ${TARGET_NAME}
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/types.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/vec3.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/vec2.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/mat4x4.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/mathutils.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/quaternion.h"
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/vec3.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/vec2.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/mat4x4.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/quaternion.cpp"
)

# set additional target properties
set_target_properties(${TARGET_NAME} PROPERTIES LINKER_LANGUAGE CXX DEBUG_POSTFIX ".debug")