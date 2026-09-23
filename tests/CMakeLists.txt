# Phase 0 smoke/regression tests: cross-platform, no window/surface required.
# Purpose is to confirm the restructured build actually produces working code, not just that
# it compiles.

add_executable(test_texture_format test_texture_format.cpp)
target_link_libraries(test_texture_format PRIVATE driver.graphics)
add_test(NAME texture_format COMMAND test_texture_format)

add_executable(test_math_smoke test_math_smoke.cpp)
target_link_libraries(test_math_smoke PRIVATE engine.shared)
add_test(NAME math_smoke COMMAND test_math_smoke)

add_executable(test_vulkan_bringup test_vulkan_bringup.cpp)
target_link_libraries(test_vulkan_bringup PRIVATE Vulkan::Vulkan)
add_test(NAME vulkan_bringup COMMAND test_vulkan_bringup)

add_executable(test_simd_transformbatch test_simd_transformbatch.cpp)
target_link_libraries(test_simd_transformbatch PRIVATE engine.shared)
add_test(NAME simd_transformbatch COMMAND test_simd_transformbatch)

add_executable(test_fixed_smoke test_fixed_smoke.cpp)
target_link_libraries(test_fixed_smoke PRIVATE engine.shared)
add_test(NAME fixed_smoke COMMAND test_fixed_smoke)

add_executable(test_physics_determinism test_physics_determinism.cpp)
target_link_libraries(test_physics_determinism PRIVATE Jolt)
add_test(NAME physics_determinism COMMAND test_physics_determinism)
