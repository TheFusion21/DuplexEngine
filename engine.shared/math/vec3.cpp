// EXTERNAL INCLUDES
// INTERNAL INCLUDES
#include "vec3.h"
#include "mat4x4.h"

using namespace Engine::Math;

const Vec3 Vec3::Zero = { static_cast<real>(0.0), static_cast<real>(0.0), static_cast<real>(0.0) };
const Vec3 Vec3::UnitX = { static_cast<real>(1.0), static_cast<real>(0.0), static_cast<real>(0.0) };
const Vec3 Vec3::UnitY= { static_cast<real>(0.0), static_cast<real>(1.0), static_cast<real>(0.0) };
const Vec3 Vec3::UnitZ = { static_cast<real>(0.0), static_cast<real>(0.0), static_cast<real>(1.0) };
const Vec3 Vec3::UnitScale = { static_cast<real>(1.0), static_cast<real>(1.0), static_cast<real>(1.0) };

Vec3& Vec3::operator*=(const Mat4x4& rhs)
{
	*this = rhs * *this;
	return *this;
}
