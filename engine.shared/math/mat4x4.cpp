
// EXTERNAL INCLUDES
// INTERNAL INCLUDES
#include "mat4x4.h"

using namespace DUPLEX_NS_MATH;

const Mat4x4 Mat4x4::Identity =
{
	static_cast<real>(1.0), static_cast<real>(0.0), static_cast<real>(0.0), static_cast<real>(0.0),
	static_cast<real>(0.0), static_cast<real>(1.0), static_cast<real>(0.0), static_cast<real>(0.0),
	static_cast<real>(0.0), static_cast<real>(0.0), static_cast<real>(1.0), static_cast<real>(0.0),
	static_cast<real>(0.0), static_cast<real>(0.0), static_cast<real>(0.0), static_cast<real>(1.0)
};
const Mat4x4 Mat4x4::Zero =
{
	static_cast<real>(0.0), static_cast<real>(0.0), static_cast<real>(0.0), static_cast<real>(0.0),
	static_cast<real>(0.0), static_cast<real>(0.0), static_cast<real>(0.0), static_cast<real>(0.0),
	static_cast<real>(0.0), static_cast<real>(0.0), static_cast<real>(0.0), static_cast<real>(0.0),
	static_cast<real>(0.0), static_cast<real>(0.0), static_cast<real>(0.0), static_cast<real>(0.0)
};
