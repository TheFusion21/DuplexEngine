#pragma once

// EXTERNAL INCLUDES
#include <cmath>
#include <cfloat>
#include <cassert>
// INTERNAL INCLUDES
#include "types.h"
#include "../namespaces.h"

namespace DUPLEX_NS_MATH
{
	class Vec2Int
	{
	public:

		int x = 0;
		int y = 0;

		static const Vec2Int Zero;
		static const Vec2Int UnitX;
		static const Vec2Int UnitY;
		static const Vec2Int UnitScale;

		inline Vec2Int(const Vec2Int& vec)
			: Vec2Int(vec.x, vec.y)
		{ }
		inline Vec2Int(int x = 0
			, int y = 0) :
			x(x),
			y(y)
		{ }

		inline float SqrMagnitude() const
		{
			return static_cast<float>(x) * x + static_cast<float>(y) * y;
		}
		inline float Distance(Vec2Int v) const
		{
#ifdef DOUBLEPRECISION
			return sqrt(SqrDistance(v));
#else
			return sqrtf(SqrDistance(v));
#endif
		}
		inline float SqrDistance(Vec2Int v) const
		{
			return (v - *this).SqrMagnitude();
		}

		inline Vec2Int& operator+=(const Vec2Int& rhs)
		{
			this->x += rhs.x;
			this->y += rhs.y;
			return *this;
		}
		inline Vec2Int& operator-=(const Vec2Int& rhs)
		{
			this->x -= rhs.x;
			this->y -= rhs.y;
			return *this;
		}
		inline Vec2Int& operator*=(const Vec2Int& rhs)
		{
			this->x *= rhs.x;
			this->y *= rhs.y;
			return *this;
		}
		inline Vec2Int& operator/=(const Vec2Int& rhs)
		{
			assert(rhs != Vec2Int::Zero);
			this->x /= rhs.x;
			this->y /= rhs.y;
			return *this;
		}
		inline Vec2Int& operator*=(const int rhs)
		{
			this->x *= rhs;
			this->y *= rhs;
			return *this;
		}
		inline Vec2Int& operator/=(const int rhs)
		{
			assert(rhs != 0);
			this->x /= rhs;
			this->y /= rhs;
			return *this;
		}

		inline Vec2Int operator+(const Vec2Int& v) const
		{
			return Vec2Int{ x + v.x, y + v.y };
		}
		inline Vec2Int operator-(const Vec2Int& v) const
		{
			return Vec2Int{ x - v.x, y - v.y };
		}
		inline Vec2Int operator*(const Vec2Int& v) const
		{
			return Vec2Int{ x * v.x, y * v.y };
		}
		inline Vec2Int operator/(const Vec2Int& v) const
		{
			assert(v != Vec2Int::Zero);
			return Vec2Int{ x / v.x, y / v.y };
		}
		inline Vec2Int operator*(const int t) const
		{
			return Vec2Int{ x * t, y * t };
		}
		inline Vec2Int operator/(const int t) const
		{
			assert(t != 0);
			return Vec2Int{ x / t, y / t };
		}

		inline bool operator==(const Vec2Int& v) const
		{
			if (x == v.x && y == v.y)
				return true;
			return false;
		}
		inline bool operator!=(const Vec2Int& v) const
		{
			return !(*this == v);
		}

		inline Vec2Int operator-()
		{
			return Vec2Int{ -x, -y };
		}
	};
}
