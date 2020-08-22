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
        "${CMAKE_CURRENT_SOURCE_DIR}/vertex.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/time.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/mesh.h"
        

        "${CMAKE_CURRENT_SOURCE_DIR}/math/vec2.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/math/vec3.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/math/mat4x4.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/math/mathutils.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/math/quaternion.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/math/types.h"

        "${CMAKE_CURRENT_SOURCE_DIR}/utils/color.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/utils/util.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/utils/exceptions.h"

        "${CMAKE_CURRENT_SOURCE_DIR}/containers/fast_mutex.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/containers/tinythread.h"

        
        # "${CMAKE_CURRENT_SOURCE_DIR}/utils/singleton.h"

    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/time.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/mesh.cpp"

        "${CMAKE_CURRENT_SOURCE_DIR}/math/vec2.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/math/vec3.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/math/mat4x4.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/math/quaternion.cpp"

        "${CMAKE_CURRENT_SOURCE_DIR}/utils/color.cpp"

        "${CMAKE_CURRENT_SOURCE_DIR}/containers/tinythread.cpp"
)
# set additional target properties
set_target_properties(${TARGET_NAME} PROPERTIES LINKER_LANGUAGE CXX DEBUG_POSTFIX ".debug")