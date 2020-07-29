# define project
project(engine.shared.utils C CXX)
SET(TARGET_NAME engine.shared.utils)

# configure executable, properties and linkage
add_library(${TARGET_NAME} STATIC)

# add sources to target
target_sources(
    ${TARGET_NAME}
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/color.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/singleton.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/util.h"
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/color.cpp"
)

# set additional target properties
set_target_properties(${TARGET_NAME} PROPERTIES LINKER_LANGUAGE CXX DEBUG_POSTFIX ".debug")