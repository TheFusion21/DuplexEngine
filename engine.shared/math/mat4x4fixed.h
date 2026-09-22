#pragma once
#include "fixed.h"
#include "vec3fixed.h"
#include "../namespaces.h"

namespace DUPLEX_NS_MATH
{
	// Deterministic fixed-point equivalent of Mat4x4 (see fixed.h). No current call sites -
	// forward-looking scaffolding for a future deterministic subsystem. Deliberately does NOT
	// support rotation (FromOrientation/mat4_cast(quat) equivalent) - that needs a
	// deterministic fixed-point sin/cos, which doesn't exist yet; only translation, scale and
	// their composition are provided. Same row/column convention as Mat4x4 (m_ij: row i,
	// column j, 1-indexed; translation in column 4) for consistency, in case this is ever
	// composed with or uploaded alongside float matrices later.
	class Mat4x4Fixed
	{
	public:
		Fixed m11, m12, m13, m14;
		Fixed m21, m22, m23, m24;
		Fixed m31, m32, m33, m34;
		Fixed m41, m42, m43, m44;

		static const Mat4x4Fixed Identity;

		Mat4x4Fixed operator*(const Mat4x4Fixed& r) const
		{
			Mat4x4Fixed out;
			out.m11 = m11 * r.m11 + m12 * r.m21 + m13 * r.m31 + m14 * r.m41;
			out.m12 = m11 * r.m12 + m12 * r.m22 + m13 * r.m32 + m14 * r.m42;
			out.m13 = m11 * r.m13 + m12 * r.m23 + m13 * r.m33 + m14 * r.m43;
			out.m14 = m11 * r.m14 + m12 * r.m24 + m13 * r.m34 + m14 * r.m44;

			out.m21 = m21 * r.m11 + m22 * r.m21 + m23 * r.m31 + m24 * r.m41;
			out.m22 = m21 * r.m12 + m22 * r.m22 + m23 * r.m32 + m24 * r.m42;
			out.m23 = m21 * r.m13 + m22 * r.m23 + m23 * r.m33 + m24 * r.m43;
			out.m24 = m21 * r.m14 + m22 * r.m24 + m23 * r.m34 + m24 * r.m44;

			out.m31 = m31 * r.m11 + m32 * r.m21 + m33 * r.m31 + m34 * r.m41;
			out.m32 = m31 * r.m12 + m32 * r.m22 + m33 * r.m32 + m34 * r.m42;
			out.m33 = m31 * r.m13 + m32 * r.m23 + m33 * r.m33 + m34 * r.m43;
			out.m34 = m31 * r.m14 + m32 * r.m24 + m33 * r.m34 + m34 * r.m44;

			out.m41 = m41 * r.m11 + m42 * r.m21 + m43 * r.m31 + m44 * r.m41;
			out.m42 = m41 * r.m12 + m42 * r.m22 + m43 * r.m32 + m44 * r.m42;
			out.m43 = m41 * r.m13 + m42 * r.m23 + m43 * r.m33 + m44 * r.m43;
			out.m44 = m41 * r.m14 + m42 * r.m24 + m43 * r.m34 + m44 * r.m44;
			return out;
		}

		bool operator==(const Mat4x4Fixed& o) const
		{
			return m11 == o.m11 && m12 == o.m12 && m13 == o.m13 && m14 == o.m14
				&& m21 == o.m21 && m22 == o.m22 && m23 == o.m23 && m24 == o.m24
				&& m31 == o.m31 && m32 == o.m32 && m33 == o.m33 && m34 == o.m34
				&& m41 == o.m41 && m42 == o.m42 && m43 == o.m43 && m44 == o.m44;
		}
		bool operator!=(const Mat4x4Fixed& o) const { return !(*this == o); }

		static Mat4x4Fixed FromTranslation(Vec3Fixed v)
		{
			Mat4x4Fixed m = Identity;
			m.m14 = v.x; m.m24 = v.y; m.m34 = v.z;
			return m;
		}
		static Mat4x4Fixed FromScale(Vec3Fixed v)
		{
			Mat4x4Fixed m;
			m.m11 = v.x; m.m22 = v.y; m.m33 = v.z; m.m44 = Fixed(1);
			return m;
		}
	};

	inline const Mat4x4Fixed Mat4x4Fixed::Identity = []() {
		Mat4x4Fixed m;
		m.m11 = Fixed(1); m.m22 = Fixed(1); m.m33 = Fixed(1); m.m44 = Fixed(1);
		return m;
	}();
}
