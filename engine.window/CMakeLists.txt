# define project
project(engine.window C CXX)
SET(TARGET_NAME engine.window)

# configure executable, properties and linkage
add_library(${TARGET_NAME} STATIC)

# add sources to target
target_sources(
    ${TARGET_NAME}
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/window.h"

    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/window.cpp"
)
target_link_libraries(${TARGET_NAME}
    engine.shared
)
target_include_directories(${TARGET_NAME} PUBLIC
	"${CMAKE_SOURCE_DIR}/engine.shared"
)
# set additional target properties
set_target_properties(${TARGET_NAME} PROPERTIES LINKER_LANGUAGE CXX DEBUG_POSTFIX ".debug")