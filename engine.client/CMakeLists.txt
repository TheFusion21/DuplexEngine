# define project
project(DuplexEngine C CXX)
SET(TARGET_NAME DuplexEngine)

cmake_minimum_required (VERSION 3.9)

# Add source to this project's executable.
add_executable(${TARGET_NAME} "../res/appicon.ico" "../res/resources.rc")

# add sources to target
target_sources(
    ${TARGET_NAME}
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/client.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/appwindow.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/flycamera.h"
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/client.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/appwindow.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/flycamera.cpp"
)

# set additional target properties
set_target_properties(${TARGET_NAME} PROPERTIES LINKER_LANGUAGE CXX DEBUG_POSTFIX ".debug")

# link libraries against the target
target_link_libraries(${TARGET_NAME}
    engine.core
    D3D11 DXGI D3DCompiler
)

# additional includes for target
target_include_directories(${TARGET_NAME} PRIVATE
	"${CMAKE_SOURCE_DIR}/engine.core"
)
