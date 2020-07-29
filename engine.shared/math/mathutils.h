#pragma once

// EXTERNAL INCLUDES
#include <cassert>
// INTERNAL INCLUDES
#include "types.h"

namespace Engine::Math
{
	constexpr real PI = static_cast<real>(3.14159265359);
	constexpr real PI_2 = PI / static_cast<real>(2.0);
	
	constexpr real AngleToRad()
	{
		return PI / static_cast<real>(180.0);
	}
	constexpr real AngleToDeg()
	{
		return static_cast<real>(180.0) / PI;
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
}