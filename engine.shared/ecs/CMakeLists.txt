# define project
project(engine.shared.ecs C CXX)
SET(TARGET_NAME engine.shared.ecs)

# configure executable, properties and linkage
add_library(${TARGET_NAME} STATIC)

# add sources to target
target_sources(
    ${TARGET_NAME}
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/entity.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/componentdata.h"
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/entity.cpp"
)

# set additional target properties
set_target_properties(${TARGET_NAME} PROPERTIES LINKER_LANGUAGE CXX DEBUG_POSTFIX ".debug")