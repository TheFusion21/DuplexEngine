#pragma once
#include "fixed.h"
#include "../namespaces.h"

namespace DUPLEX_NS_MATH
{
	// Deterministic fixed-point equivalent of Vec3 (see fixed.h for why). No current call
	// sites - forward-looking scaffolding for a future deterministic subsystem.
	class Vec3Fixed
	{
	public:
		Fixed x, y, z;

		constexpr Vec3Fixed() = default;
		constexpr Vec3Fixed(Fixed x, Fixed y, Fixed z) : x(x), y(y), z(z) {}

		static const Vec3Fixed Zero;
		static const Vec3Fixed UnitX;
		static const Vec3Fixed UnitY;
		static const Vec3Fixed UnitZ;
		static const Vec3Fixed UnitScale;

		Vec3Fixed operator+(const Vec3Fixed& o) const { return { x + o.x, y + o.y, z + o.z }; }
		Vec3Fixed operator-(const Vec3Fixed& o) const { return { x - o.x, y - o.y, z - o.z }; }
		Vec3Fixed operator-() const { return { -x, -y, -z }; }
		Vec3Fixed operator*(Fixed t) const { return { x * t, y * t, z * t }; }
		Vec3Fixed operator*(const Vec3Fixed& o) const { return { x * o.x, y * o.y, z * o.z }; }

		Vec3Fixed& operator+=(const Vec3Fixed& o) { x += o.x; y += o.y; z += o.z; return *this; }
		Vec3Fixed& operator-=(const Vec3Fixed& o) { x -= o.x; y -= o.y; z -= o.z; return *this; }

		bool operator==(const Vec3Fixed& o) const { return x == o.x && y == o.y && z == o.z; }
		bool operator!=(const Vec3Fixed& o) const { return !(*this == o); }

		Fixed Dot(const Vec3Fixed& o) const { return x * o.x + y * o.y + z * o.z; }
		Vec3Fixed Cross(const Vec3Fixed& o) const
		{
			return { y * o.z - z * o.y, z * o.x - x * o.z, x * o.y - y * o.x };
		}
		Fixed SqrMagnitude() const { return Dot(*this); }
		Fixed Magnitude() const { return Fixed::Sqrt(SqrMagnitude()); }

		// Returns false (leaving *this unchanged) rather than dividing by zero, same contract
		// as the old float Vec3::Normalize().
		bool Normalize()
		{
			Fixed mag = Magnitude();
			if (mag == Fixed())
				return false;
			x = x / mag; y = y / mag; z = z / mag;
			return true;
		}
	};

	inline const Vec3Fixed Vec3Fixed::Zero = { Fixed(0), Fixed(0), Fixed(0) };
	inline const Vec3Fixed Vec3Fixed::UnitX = { Fixed(1), Fixed(0), Fixed(0) };
	inline const Vec3Fixed Vec3Fixed::UnitY = { Fixed(0), Fixed(1), Fixed(0) };
	inline const Vec3Fixed Vec3Fixed::UnitZ = { Fixed(0), Fixed(0), Fixed(1) };
	inline const Vec3Fixed Vec3Fixed::UnitScale = { Fixed(1), Fixed(1), Fixed(1) };
}
