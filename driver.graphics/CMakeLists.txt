# define project
project(driver.graphics C CXX)
SET(TARGET_NAME driver.graphics)

# configure executable, properties and linkage
add_library(${TARGET_NAME} STATIC)

# add sources to target
target_sources(
    ${TARGET_NAME}
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/renderer.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/shadercb.h"
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/renderer.cpp"
)

# set additional target properties
set_target_properties(${TARGET_NAME} PROPERTIES LINKER_LANGUAGE CXX DEBUG_POSTFIX ".debug")

target_link_libraries(${TARGET_NAME}
    engine.shared
)
target_include_directories(${TARGET_NAME} PUBLIC
	"${CMAKE_SOURCE_DIR}/engine.shared"
)