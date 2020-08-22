#pragma once

// EXTERNAL INCLUDES
#include <cassert>
// INTERNAL INCLUDES
#include "types.h"

namespace Engine::Math
{
	constexpr real PI = static_cast<real>(3.1415926535897932384626433832795);
	constexpr real PI_2 = PI / static_cast<real>(2.0);
	
	constexpr real AngleToRad()
	{
		return static_cast<real>(0.01745329251994329576923690768489);
	}
	constexpr real AngleToDeg()
	{
		return static_cast<real>(57.295779513082320876798154814105);
	}

	template<typename T>
	static constexpr T Max(const T& val, const T& max)
	{
		return (val > max) * val + (val <= max) * max;
	}
	template<typename T>
	static constexpr T Min(const T& val, const T& min)
	{
		return (val > min) * min + (val <= min) * val;
	}
	template<typename T>
	static constexpr T Clamp(const T& val, const T& min, const T& max)
	{
		assert(min <= max);
		return Max(Min(val, max), min);
	}
	template<typename T>
	static constexpr T Lerp(const T& a, const T& b, const T& t)
	{
		return a + t * (b - a);
	}
	template<typename T>
	static constexpr T MipMapCountFrom(T width, T height)
	{
		T levels = 1;
		while ((width | height) >> levels) {
			++levels;
		}
		return levels;
	}
}