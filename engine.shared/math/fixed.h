#pragma once
#include <cstdint>
#include "types.h"
#include "../namespaces.h"

namespace DUPLEX_NS_MATH
{
	// Deterministic Q15.16 fixed-point scalar: i32 storage, 16 fractional bits. For simulation
	// code that needs bit-identical results across different machines/compilers/optimization
	// levels - something plain IEEE float/double cannot reliably guarantee (FMA contraction
	// changing rounding, differing libm implementations for transcendental functions, etc).
	// Not used by any current system - this is scaffolding for whatever deterministic
	// subsystem (netcode, replay, ...) needs it later. Range is +-32767.99998; there is
	// deliberately no rotation/quaternion support yet (needs a deterministic fixed-point
	// trig implementation, which is a separate, non-trivial piece of work) - see Vec3Fixed and
	// Mat4x4Fixed below for what that limits.
	class Fixed
	{
	public:
		static constexpr int FracBits = 16;
		static constexpr i32 Scale = 1 << FracBits;

		i32 raw = 0;

		constexpr Fixed() = default;
		constexpr Fixed(i32 integerValue) : raw(integerValue * Scale) {}

		// Not constexpr (no compile-time float-to-fixed conversion needed anywhere yet, and
		// keeping the conversion simple/obviously-correct matters more here than compile-time
		// evaluability).
		explicit Fixed(float value) : raw(static_cast<i32>(value * static_cast<float>(Scale))) {}
		explicit Fixed(double value) : raw(static_cast<i32>(value * static_cast<double>(Scale))) {}

		static constexpr Fixed FromRaw(i32 rawValue)
		{
			Fixed f;
			f.raw = rawValue;
			return f;
		}

		float ToFloat() const { return static_cast<float>(raw) / static_cast<float>(Scale); }
		double ToDouble() const { return static_cast<double>(raw) / static_cast<double>(Scale); }
		i32 ToInt() const { return raw / Scale; }

		Fixed operator+(Fixed o) const { return FromRaw(raw + o.raw); }
		Fixed operator-(Fixed o) const { return FromRaw(raw - o.raw); }
		Fixed operator-() const { return FromRaw(-raw); }

		// i32*i32 widened to i64 before shifting back down, so the intermediate can't overflow
		// for any pair of representable Fixed values.
		Fixed operator*(Fixed o) const
		{
			i64 wide = static_cast<i64>(raw) * static_cast<i64>(o.raw);
			return FromRaw(static_cast<i32>(wide >> FracBits));
		}
		Fixed operator/(Fixed o) const
		{
			i64 wide = (static_cast<i64>(raw) << FracBits) / o.raw;
			return FromRaw(static_cast<i32>(wide));
		}

		Fixed& operator+=(Fixed o) { raw += o.raw; return *this; }
		Fixed& operator-=(Fixed o) { raw -= o.raw; return *this; }
		Fixed& operator*=(Fixed o) { *this = *this * o; return *this; }
		Fixed& operator/=(Fixed o) { *this = *this / o; return *this; }

		bool operator==(Fixed o) const { return raw == o.raw; }
		bool operator!=(Fixed o) const { return raw != o.raw; }
		bool operator<(Fixed o) const { return raw < o.raw; }
		bool operator<=(Fixed o) const { return raw <= o.raw; }
		bool operator>(Fixed o) const { return raw > o.raw; }
		bool operator>=(Fixed o) const { return raw >= o.raw; }

		static Fixed Abs(Fixed v) { return FromRaw(v.raw < 0 ? -v.raw : v.raw); }

		// Pure-integer bit-by-bit square root (no floating point anywhere in this path) - the
		// classic binary digit-by-digit isqrt algorithm, applied to the value widened by
		// FracBits first so the result comes out already in Q15.16 - deterministic on any
		// conforming integer ALU, unlike floating-point sqrt (which - while IEEE 754 mandates
		// correctly-rounded results - this avoids relying on that guarantee at all).
		static Fixed Sqrt(Fixed v)
		{
			if (v.raw <= 0)
				return Fixed();

			i64 n = static_cast<i64>(v.raw) << FracBits;
			i64 result = 0;
			i64 bit = i64(1) << 62;
			while (bit > n)
				bit >>= 2;
			while (bit != 0)
			{
				i64 candidate = result + bit;
				if (n >= candidate)
				{
					n -= candidate;
					result = (result >> 1) + bit;
				}
				else
				{
					result >>= 1;
				}
				bit >>= 2;
			}
			return FromRaw(static_cast<i32>(result));
		}
	};
}
