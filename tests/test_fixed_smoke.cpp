// Regression tests for the deterministic fixed-point math scaffolding (math/fixed.h,
// vec3fixed.h, mat4x4fixed.h). No engine system uses this yet; these checks exist so the
// arithmetic (especially the pure-integer Fixed::Sqrt, which is easy to get subtly wrong) is
// verified before anything is built on top of it.

#include "math/fixed.h"
#include "math/vec3fixed.h"
#include "math/mat4x4fixed.h"
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

	bool ApproxEqual(float a, float b, float epsilon = 1e-3f)
	{
		return std::fabs(a - b) <= epsilon;
	}
}

int main()
{
	// Basic arithmetic round-trips through float conversion.
	Fixed a(3.5f);
	Fixed b(2.25f);
	CheckTrue(ApproxEqual((a + b).ToFloat(), 5.75f), "3.5 + 2.25 == 5.75");
	CheckTrue(ApproxEqual((a - b).ToFloat(), 1.25f), "3.5 - 2.25 == 1.25");
	CheckTrue(ApproxEqual((a * b).ToFloat(), 7.875f), "3.5 * 2.25 == 7.875");
	CheckTrue(ApproxEqual((a / b).ToFloat(), 1.5555555f, 1e-2f), "3.5 / 2.25 ~= 1.5556");

	// Negative values.
	Fixed negA(-4.0f);
	CheckTrue(ApproxEqual((negA + a).ToFloat(), -0.5f), "-4 + 3.5 == -0.5");
	CheckTrue(Fixed::Abs(negA) == Fixed(4.0f), "Abs(-4) == 4");

	// Integer sqrt: perfect squares must be exact, others approximately correct.
	CheckTrue(Fixed::Sqrt(Fixed(16.0f)) == Fixed(4.0f), "Sqrt(16) == 4");
	CheckTrue(Fixed::Sqrt(Fixed(0.0f)) == Fixed(0.0f), "Sqrt(0) == 0");
	CheckTrue(ApproxEqual(Fixed::Sqrt(Fixed(2.0f)).ToFloat(), 1.41421356f, 1e-3f), "Sqrt(2) ~= 1.41421356");
	CheckTrue(ApproxEqual(Fixed::Sqrt(Fixed(12345.6789f)).ToFloat(), std::sqrt(12345.6789f), 0.02f), "Sqrt(12345.6789) matches std::sqrt within fixed-point precision");

	// Vec3Fixed: same handedness/identity invariants as the float Vec3 smoke test.
	CheckTrue(Vec3Fixed::UnitX.Cross(Vec3Fixed::UnitY) == Vec3Fixed::UnitZ, "UnitX x UnitY == UnitZ (fixed)");
	CheckTrue(Vec3Fixed::UnitX.Dot(Vec3Fixed::UnitX) == Fixed(1), "UnitX . UnitX == 1 (fixed)");

	Vec3Fixed v(Fixed(3.0f), Fixed(4.0f), Fixed(0.0f));
	CheckTrue(v.Magnitude() == Fixed(5.0f), "|(3,4,0)| == 5 (fixed, exact 3-4-5 triangle)");
	Vec3Fixed vCopy = v;
	CheckTrue(vCopy.Normalize(), "Normalize succeeds for a non-zero vector");
	CheckTrue(ApproxEqual(vCopy.Magnitude().ToFloat(), 1.0f, 1e-2f), "normalized vector has unit length");

	Vec3Fixed zero = Vec3Fixed::Zero;
	CheckTrue(!zero.Normalize(), "Normalize on a zero vector fails rather than dividing by zero");

	// Mat4x4Fixed: identity and TRS (translate * scale, no rotation support yet) composition.
	CheckTrue(Mat4x4Fixed::Identity * Mat4x4Fixed::Identity == Mat4x4Fixed::Identity, "Identity * Identity == Identity (fixed)");

	Mat4x4Fixed t = Mat4x4Fixed::FromTranslation(Vec3Fixed(Fixed(1), Fixed(2), Fixed(3)));
	Mat4x4Fixed s = Mat4x4Fixed::FromScale(Vec3Fixed(Fixed(2), Fixed(2), Fixed(2)));
	Mat4x4Fixed ts = t * s;
	CheckTrue(ts.m11 == Fixed(2) && ts.m22 == Fixed(2) && ts.m33 == Fixed(2), "translate*scale keeps scale on the diagonal");
	CheckTrue(ts.m14 == Fixed(1) && ts.m24 == Fixed(2) && ts.m34 == Fixed(3), "translate*scale keeps translation in the last column");

	if (failures == 0)
	{
		std::printf("test_fixed_smoke: all checks passed\n");
		return 0;
	}
	std::fprintf(stderr, "test_fixed_smoke: %d check(s) failed\n", failures);
	return 1;
}
