#pragma once

// EXTERNAL INCLUDES
#include <cmath>
#include <cfloat>
#include <cassert>
// INTERNAL INCLUDES
#include "types.h"

namespace Engine::Math
{
	class Vec2
	{
	public:

		real x = static_cast<real>(0.0);
		real y = static_cast<real>(0.0);

		static const Vec2 Zero;
		static const Vec2 UnitX;
		static const Vec2 UnitY;
		static const Vec2 UnitScale;

		inline Vec2(const Vec2& vec)
			: Vec2(vec.x, vec.y)
		{ }
		inline Vec2(real x = static_cast<real>(0.0)
			, real y = static_cast<real>(0.0)) :
			x(x),
			y(y)
		{ }

		inline bool Normalize()
		{
			real invMag = static_cast<real>(1.0) / Magnitude();

#ifdef DOUBLEPRECISION 
			if (invMag >= DBL_MIN)
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
			return x * x + y * y;
		}

		inline real Dot(Vec2 v) const
		{
			return	(x * v.x) +
					(y * v.y);
		}
		inline real Cross(Vec2 v) const
		{
			return x * v.y - y * v.x;
		}
		inline Vec2 Lerp(Vec2 v, real t) const
		{
			return (*this) + (v - (*this)) * t;
		}
		inline real Distance(Vec2 v) const
		{
#ifdef DOUBLEPRECISION
			return sqrt(SqrDistance(v));
#else
			return sqrtf(SqrDistance(v));
#endif
		}
		inline real SqrDistance(Vec2 v) const
		{
			return (v - *this).SqrMagnitude();
		}

		inline Vec2& operator+=(const Vec2& rhs)
		{
			this->x += rhs.x;
			this->y += rhs.y;
			return *this;
		}
		inline Vec2& operator-=(const Vec2& rhs)
		{
			this->x -= rhs.x;
			this->y -= rhs.y;
			return *this;
		}
		inline Vec2& operator*=(const Vec2& rhs)
		{
			this->x *= rhs.x;
			this->y *= rhs.y;
			return *this;
		}
		inline Vec2& operator/=(const Vec2& rhs)
		{
			assert(rhs != Vec2::Zero);
			this->x /= rhs.x;
			this->y /= rhs.y;
			return *this;
		}
		inline Vec2& operator*=(const real rhs)
		{
			this->x *= rhs;
			this->y *= rhs;
			return *this;
		}
		inline Vec2& operator/=(const real rhs)
		{
			assert(rhs != static_cast<real>(0.0));
			this->x /= rhs;
			this->y /= rhs;
			return *this;
		}

		inline Vec2 operator+(const Vec2& v) const
		{
			return Vec2{ x + v.x, y + v.y};
		}
		inline Vec2 operator-(const Vec2& v) const
		{
			return Vec2{ x - v.x, y - v.y};
		}
		inline Vec2 operator*(const Vec2& v) const
		{
			return Vec2{ x * v.x, y * v.y};
		}
		inline Vec2 operator/(const Vec2& v) const
		{
			assert(v != Vec2::Zero);
			return Vec2{ x / v.x, y / v.y};
		}
		inline Vec2 operator*(const real t) const
		{
			return Vec2{ x * t, y * t};
		}
		inline Vec2 operator/(const real t) const
		{
			assert(t != static_cast<real>(0.0));
			return Vec2{ x / t, y / t};
		}

		inline bool operator==(const Vec2& v) const
		{
			if (x == v.x && y == v.y)
				return true;
			return false;
		}
		inline bool operator!=(const Vec2& v) const
		{
			return !(*this == v);
		}

		inline Vec2 operator-()
		{
			return Vec2{ -x, -y};
		}
	};
}
