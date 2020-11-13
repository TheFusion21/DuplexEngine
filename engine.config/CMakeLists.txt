# define project
project(engine.config C CXX)
SET(TARGET_NAME engine.config)

# configure executable, propertiesand linkage
add_library(${TARGET_NAME} STATIC)

# add sources to target
target_sources(
    ${TARGET_NAME}
    PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/engineconfig.h"

    PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/engineconfig.cpp"
)
target_link_libraries(${TARGET_NAME}
    engine.shared
)
target_include_directories(${TARGET_NAME} PUBLIC
    "${CMAKE_SOURCE_DIR}/engine.shared"
)
# set additional target properties
set_target_properties(${TARGET_NAME} PROPERTIES LINKER_LANGUAGE CXX DEBUG_POSTFIX ".debug")