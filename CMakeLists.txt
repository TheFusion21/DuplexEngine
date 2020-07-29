# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.8)

project ("PR210")

include(compiler.cmake)

# Include sub-projects.
add_subdirectory ("engine.shared")
add_subdirectory ("engine.shared/utils")
add_subdirectory ("engine.shared/math")
add_subdirectory ("engine.shared/ecs")
add_subdirectory ("engine.client")

