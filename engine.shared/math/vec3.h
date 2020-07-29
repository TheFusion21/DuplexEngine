#pragma once

// EXTERNAL INCLUDES
#include <cmath>
#include <cfloat>
#include <cassert>
// INTERNAL INCLUDES
#include "types.h"

namespace Engine::Math
{
	class Vec3
	{
	public:
		
		real x = static_cast<real>(0.0);
		real y = static_cast<real>(0.0);
		real z = static_cast<real>(0.0);
		
		static const Vec3 Zero;
		static const Vec3 UnitX;
		static const Vec3 UnitY;
		static const Vec3 UnitZ;
		static const Vec3 UnitScale;

		inline Vec3(const Vec3& vec)
			: Vec3(vec.x, vec.y, vec.z)
		{ }
		inline Vec3(real x = static_cast<real>(0.0)
			, real y = static_cast<real>(0.0)
			, real z = static_cast<real>(0.0)) :
			x(x),
			y(y),
			z(z)
		{ }

		inline bool Normalize()
		{
			real invMag = static_cast<real>(1.0) / Magnitude();
			
#ifdef DOUBLEPRECISION 
			if(invMag >= DBL_MIN)
#else
			if (invMag >= FLT_MIN)
#endif // DOUBLEPRECISION 
			{
				*this *= invMag;
				return true;
			}

			return false;
		}
		inline real Magnitude() const
		{
			#ifdef DOUBLEPRECISION
				return sqrt(SqrMagnitude());
			#else
				return sqrtf(SqrMagnitude());
			#endif
		}
		inline real SqrMagnitude() const
		{
			return x * x + y * y + z * z;
		}

		inline real Dot(Vec3 v) const
		{
			return	(x * v.x) +
					(y * v.y) +
					(z * v.z);
		}
		inline Vec3 Cross(Vec3 v) const
		{
			return Vec3{y * v.z - z * v.y,
						z * v.x - z * v.x,
						x * v.y - y * v.x };
		}
		inline Vec3 Lerp(Vec3 v, real t) const
		{
			return (*this) + (v - (*this)) * t;
		}
		inline real Distance(Vec3 v) const
		{
#ifdef DOUBLEPRECISION
			return sqrt(SqrDistance(v));
#else
			return sqrtf(SqrDistance(v));
#endif
		}
		inline real SqrDistance(Vec3 v) const
		{
			return (v - *this).SqrMagnitude();
		}

		inline Vec3& operator+=(const Vec3& rhs)
		{
			this->x += rhs.x;
			this->y += rhs.y;
			this->z += rhs.z;
			return *this;
		}
		inline Vec3& operator-=(const Vec3& rhs)
		{
			this->x -= rhs.x;
			this->y -= rhs.y;
			this->z -= rhs.z;
			return *this;
		}
		inline Vec3& operator*=(const Vec3& rhs)
		{
			this->x *= rhs.x;
			this->y *= rhs.y;
			this->z *= rhs.z;
			return *this;
		}
		inline Vec3& operator/=(const Vec3& rhs)
		{
			assert(rhs != Vec3::Zero);
			this->x /= rhs.x;
			this->y /= rhs.y;
			this->z /= rhs.z;
			return *this;
		}
		inline Vec3& operator*=(const real rhs)
		{
			this->x *= rhs;
			this->y *= rhs;
			this->z *= rhs;
			return *this;
		}
		Vec3& operator*=(const class Mat4x4& rhs);

		inline Vec3& operator/=(const real rhs)
		{
			assert(rhs != static_cast<real>(0.0));
			this->x /= rhs;
			this->y /= rhs;
			this->z /= rhs;
			return *this;
		}

		inline Vec3 operator+(const Vec3& v) const
		{
			return Vec3{ x + v.x, y + v.y, z + v.z };
		}
		inline Vec3 operator-(const Vec3& v) const
		{
			return Vec3{ x - v.x, y - v.y, z - v.z };
		}
		inline Vec3 operator*(const Vec3& v) const
		{
			return Vec3{ x * v.x, y * v.y, z * v.z };
		}
		inline Vec3 operator/(const Vec3& v) const
		{
			assert(v != Vec3::Zero);
			return Vec3{ x / v.x, y / v.y, z / v.z };
		}
		inline Vec3 operator*(const real t) const
		{
			return Vec3{ x * t, y * t, z * t };
		}
		inline Vec3 operator/(const real t) const
		{
			assert(t != static_cast<real>(0.0));
			return Vec3{ x / t, y / t, z / t };
		}

		inline bool operator==(const Vec3& v) const
		{
			if (x == v.x && y == v.y && z == v.z)
				return true;
			return false;
		}
		inline bool operator!=(const Vec3& v) const
		{
			return !(*this == v);
		}

		inline Vec3 operator-() const
		{
			return Vec3{ -x, -y, -z };
		}
	};
}
