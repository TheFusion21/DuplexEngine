# define project
project(driver.vulkan C CXX)
SET(TARGET_NAME driver.vulkan)

# configure executable, properties and linkage
add_library(${TARGET_NAME} STATIC)

# add sources to target
target_sources(
    ${TARGET_NAME}
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/vulkanrenderer.h"

    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/vulkanrenderer.cpp"
)

# set additional target properties
set_target_properties(${TARGET_NAME} PROPERTIES LINKER_LANGUAGE CXX DEBUG_POSTFIX ".debug")

target_link_libraries(${TARGET_NAME}
    driver.graphics
    "${CMAKE_SOURCE_DIR}/externalLibs/vulkan-1.lib"
)

target_include_directories(${TARGET_NAME} PUBLIC
	"${CMAKE_SOURCE_DIR}/driver.graphics"
)