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
        "${CMAKE_CURRENT_SOURCE_DIR}/transform.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/time.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/vertex.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/gameobject.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/component.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/mesh.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/input.h"
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/time.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/gameobject.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/component.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/mesh.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/transform.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/input.cpp"
)

# set additional target properties
set_target_properties(${TARGET_NAME} PROPERTIES LINKER_LANGUAGE CXX DEBUG_POSTFIX ".debug")