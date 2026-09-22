// Baseline regression tests for the hand-rolled math library (Vec3/Mat4x4/Quaternion).
// This exists mainly as a safety net ahead of a planned migration to GLM: these are the
// invariants a drop-in replacement needs to preserve (handedness, identity behavior,
// rotation direction), captured now while the current implementation is the known-good one.

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

	bool ApproxEqual(real a, real b, real epsilon = static_cast<real>(1e-4))
	{
		return std::fabs(a - b) <= epsilon;
	}

	bool ApproxEqual(const Vec3& a, const Vec3& b, real epsilon = static_cast<real>(1e-4))
	{
		return ApproxEqual(a.x, b.x, epsilon) && ApproxEqual(a.y, b.y, epsilon) && ApproxEqual(a.z, b.z, epsilon);
	}
}

int main()
{
	// Cross product handedness: X cross Y must be Z (right-handed).
	CheckTrue(Vec3::UnitX.Cross(Vec3::UnitY) == Vec3::UnitZ, "UnitX x UnitY == UnitZ");

	// Dot product basics.
	CheckTrue(Vec3::UnitX.Dot(Vec3::UnitX) == static_cast<real>(1.0), "UnitX . UnitX == 1");
	CheckTrue(Vec3::UnitX.Dot(Vec3::UnitY) == static_cast<real>(0.0), "UnitX . UnitY == 0");

	// Componentwise addition.
	CheckTrue(Vec3(1, 2, 3) + Vec3(4, 5, 6) == Vec3(5, 7, 9), "Vec3(1,2,3) + Vec3(4,5,6) == Vec3(5,7,9)");

	// Identity matrix must be a no-op both as a matrix product and applied to a vector.
	CheckTrue(Mat4x4::Identity * Mat4x4::Identity == Mat4x4::Identity, "Identity * Identity == Identity");
	CheckTrue(Mat4x4::Identity * Vec3::UnitX == Vec3::UnitX, "Identity * UnitX == UnitX");

	// A zero-angle rotation must be the identity quaternion.
	Quaternion noRotation = Quaternion::FromAngleAxis(static_cast<real>(0.0), Vec3::UnitY);
	CheckTrue(noRotation == Quaternion(0, 0, 0, 1), "FromAngleAxis(0, axis) == identity quaternion");

	// A +90 degree rotation about Z must take +X to +Y (right-hand rule).
	Quaternion rot90Z = Quaternion::FromAngleAxis(static_cast<real>(90.0), Vec3::UnitZ);
	Vec3 rotated = rot90Z * Vec3::UnitX;
	CheckTrue(ApproxEqual(rotated, Vec3::UnitY), "90 degree rotation about Z takes UnitX to UnitY");

	if (failures == 0)
	{
		std::printf("test_math_smoke: all checks passed\n");
		return 0;
	}
	std::fprintf(stderr, "test_math_smoke: %d check(s) failed\n", failures);
	return 1;
}
