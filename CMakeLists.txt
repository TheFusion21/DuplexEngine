# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.8)

project ("PR210")

include(compiler.cmake)

# Include sub-projects.
add_subdirectory ("driver.graphics")
add_subdirectory ("driver.d3d")
add_subdirectory ("driver.vulkan")
add_subdirectory ("engine.shared")
add_subdirectory ("engine.core")
add_subdirectory ("engine.client")


