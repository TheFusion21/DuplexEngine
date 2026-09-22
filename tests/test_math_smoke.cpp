// Baseline regression tests for the math library, now GLM-backed (see math/vec3.h,
// math/mat4x4.h, math/quaternion.h). These check the same invariants the old hand-rolled
// implementation was verified against before the GLM migration (handedness, identity
// behavior, rotation direction) - GLM defaults to the same right-handed convention, but this
// pins it down rather than assuming.

#include "math/vec3.h"
#include "math/mat4x4.h"
#include "math/quaternion.h"
#include <cmath>
#include <cstdio>

using namespace DUPLEX_NS_MATH;

namespace
{
	int failures = 0;

	void CheckTrue(bool condition, const char* label)
	{
		if (!condition)
		{
			std::fprintf(stderr, "FAIL: %s\n", label);
			failures++;
		}
	}

	bool ApproxEqual(float a, float b, float epsilon = 1e-4f)
	{
		return std::fabs(a - b) <= epsilon;
	}

	bool ApproxEqual(const Vec3& a, const Vec3& b, float epsilon = 1e-4f)
	{
		return ApproxEqual(a.x, b.x, epsilon) && ApproxEqual(a.y, b.y, epsilon) && ApproxEqual(a.z, b.z, epsilon);
	}
}

int main()
{
	// Cross product handedness: X cross Y must be Z (right-handed).
	CheckTrue(glm::cross(Vec3UnitX, Vec3UnitY) == Vec3UnitZ, "UnitX x UnitY == UnitZ");

	// Dot product basics.
	CheckTrue(glm::dot(Vec3UnitX, Vec3UnitX) == 1.0f, "UnitX . UnitX == 1");
	CheckTrue(glm::dot(Vec3UnitX, Vec3UnitY) == 0.0f, "UnitX . UnitY == 0");

	// Componentwise addition.
	CheckTrue(Vec3(1, 2, 3) + Vec3(4, 5, 6) == Vec3(5, 7, 9), "Vec3(1,2,3) + Vec3(4,5,6) == Vec3(5,7,9)");

	// Identity matrix must be a no-op both as a matrix product and applied to a vector.
	CheckTrue(Mat4x4Identity * Mat4x4Identity == Mat4x4Identity, "Identity * Identity == Identity");
	CheckTrue(Vec3(Mat4x4Identity * glm::vec4(Vec3UnitX, 1.0f)) == Vec3UnitX, "Identity * UnitX == UnitX");

	// A zero-angle rotation must be the identity quaternion.
	Quaternion noRotation = glm::angleAxis(0.0f, Vec3UnitY);
	CheckTrue(noRotation == QuatIdentity, "angleAxis(0, axis) == identity quaternion");

	// A +90 degree rotation about Z must take +X to +Y (right-hand rule).
	Quaternion rot90Z = glm::angleAxis(glm::radians(90.0f), Vec3UnitZ);
	Vec3 rotated = rot90Z * Vec3UnitX;
	CheckTrue(ApproxEqual(rotated, Vec3UnitY), "90 degree rotation about Z takes UnitX to UnitY");

	// QuaternionFromEuler (Z, then Y, then X, degrees) - the one thing GLM doesn't provide
	// directly and this migration added a small helper for. A pure +90 yaw (Y) should behave
	// the same as glm::angleAxis(90deg, UnitY) alone.
	Quaternion yaw90 = QuaternionFromEuler(Vec3(0.0f, 90.0f, 0.0f));
	Quaternion yaw90Direct = glm::angleAxis(glm::radians(90.0f), Vec3UnitY);
	CheckTrue(ApproxEqual(yaw90 * Vec3UnitX, yaw90Direct * Vec3UnitX), "QuaternionFromEuler(0,90,0) matches angleAxis(90deg, UnitY)");

	if (failures == 0)
	{
		std::printf("test_math_smoke: all checks passed\n");
		return 0;
	}
	std::fprintf(stderr, "test_math_smoke: %d check(s) failed\n", failures);
	return 1;
}
