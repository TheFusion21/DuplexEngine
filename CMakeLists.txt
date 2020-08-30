# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.8)

project ("PR210")

include(compiler.cmake)
find_package(Vulkan)
message(STATUS ${Vulkan_INCLUDE_DIR})
message(STATUS ${Vulkan_LIBRARY})

# Include sub-projects.
add_subdirectory ("driver.graphics")
add_subdirectory ("driver.d3d")
add_subdirectory ("driver.vulkan")
add_subdirectory ("engine.shared")
add_subdirectory ("engine.core")
add_subdirectory ("engine.client")

execute_process(COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/res/shadercompiler.bat")
