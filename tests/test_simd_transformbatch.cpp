// Verifies the AVX2+FMA batch world-matrix path (simd/transformbatch_avx2.cpp) produces the
// same results as the portable baseline (simd/transformbatch_baseline.cpp), which is itself
// just glm::translate/mat4_cast/scale composed the same way every ECS system already did
// per-entity. Hand-written SIMD is exactly the kind of code that's easy to get subtly wrong
// (wrong lane, wrong sign, a transposed element) without compiler help, so this checks every
// element of every matrix against the baseline for a batch that isn't a multiple of 8 (to
// exercise both the vectorized main loop and the scalar tail) and includes non-trivial
// rotations/scales/positions, not just identity cases.
//
// Runs unconditionally: on hardware without AVX2, ComputeWorldMatricesBatchAVX2 is still
// directly callable and correct (it's plain portable AVX2 intrinsics code, no different from
// any other function in the binary at compile time - only ComputeWorldMatricesBatch's runtime
// dispatch decides whether it's safe to jump into from the *baseline* build), so this can check
// it unconditionally without needing this CI/dev machine's CPU to support AVX2 itself. What it
// cannot check on non-AVX2 hardware is that the dispatcher's cpu-feature gate is itself
// correct - only that the AVX2 code, once entered, computes the right answer.

#include "simd/transformbatch.h"
#include "simd/cpufeatures.h"
#include "math/vec3.h"
#include "math/quaternion.h"
#include "math/mat4x4.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>

using namespace DUPLEX_NS_MATH;
using namespace DUPLEX_NS_SIMD;

namespace
{
	int failures = 0;

	bool ApproxEqual(const Mat4x4& a, const Mat4x4& b, float epsilon = 1e-3f)
	{
		for (int col = 0; col < 4; col++)
		{
			for (int row = 0; row < 4; row++)
			{
				if (std::fabs(a[col][row] - b[col][row]) > epsilon)
					return false;
			}
		}
		return true;
	}
}

int main()
{
	// Deterministic pseudo-random inputs (no <random> dependency needed for this).
	unsigned int seed = 12345u;
	auto nextFloat = [&seed](float lo, float hi) -> float {
		seed = seed * 1664525u + 1013904223u;
		float t = static_cast<float>(seed) / static_cast<float>(0xFFFFFFFFu);
		return lo + t * (hi - lo);
	};

	// 19 = not a multiple of 8, so this exercises both the AVX2 8-wide main loop (2 full
	// groups) and the 3-element scalar tail.
	const unsigned int count = 19;
	std::vector<Vec3> positions(count);
	std::vector<Quaternion> rotations(count);
	std::vector<Vec3> scales(count);

	for (unsigned int i = 0; i < count; i++)
	{
		positions[i] = Vec3(nextFloat(-100.0f, 100.0f), nextFloat(-100.0f, 100.0f), nextFloat(-100.0f, 100.0f));
		Vec3 axis = glm::normalize(Vec3(nextFloat(-1.0f, 1.0f), nextFloat(-1.0f, 1.0f), nextFloat(-1.0f, 1.0f) + 0.01f));
		rotations[i] = glm::angleAxis(glm::radians(nextFloat(-359.0f, 359.0f)), axis);
		scales[i] = Vec3(nextFloat(0.01f, 10.0f), nextFloat(0.01f, 10.0f), nextFloat(0.01f, 10.0f));
	}

	std::vector<Mat4x4> baseline(count);
	std::vector<Mat4x4> avx2(count);
	detail::ComputeWorldMatricesBatchBaseline(positions.data(), rotations.data(), scales.data(), baseline.data(), count);
	detail::ComputeWorldMatricesBatchAVX2(positions.data(), rotations.data(), scales.data(), avx2.data(), count);

	for (unsigned int i = 0; i < count; i++)
	{
		if (!ApproxEqual(baseline[i], avx2[i]))
		{
			std::fprintf(stderr, "FAIL: AVX2 result for entry %u doesn't match baseline\n", i);
			failures++;
		}
	}

	// The public dispatcher must produce the same result as whichever path it picks, on
	// whatever CPU this test actually runs on.
	std::vector<Mat4x4> dispatched(count);
	ComputeWorldMatricesBatch(positions.data(), rotations.data(), scales.data(), dispatched.data(), count);
	for (unsigned int i = 0; i < count; i++)
	{
		bool matchesBaseline = ApproxEqual(baseline[i], dispatched[i]);
		if (!matchesBaseline)
		{
			std::fprintf(stderr, "FAIL: dispatched result for entry %u doesn't match baseline\n", i);
			failures++;
		}
	}

	// count == 0 must be a safe no-op (both loops in ComputeWorldMatricesBatchAVX2 need to
	// handle this without reading out of bounds).
	ComputeWorldMatricesBatch(positions.data(), rotations.data(), scales.data(), dispatched.data(), 0);

	if (failures == 0)
	{
		std::printf("test_simd_transformbatch: all checks passed (AVX2 available on this CPU: %s)\n", GetCpuFeatures().avx2 && GetCpuFeatures().fma3 ? "yes" : "no");
		return 0;
	}
	std::fprintf(stderr, "test_simd_transformbatch: %d check(s) failed\n", failures);
	return 1;
}
