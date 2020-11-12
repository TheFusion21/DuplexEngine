#pragma once

// EXTERNAL INCLUDES
// INTERNAL INCLUDES
#include "math/vec3.h"
#include "math/vec2.h"
#include "utils/color.h"
#include "math/quaternion.h"
struct Vertex
{
	DUPLEX_NS_MATH::Vec3 position = DUPLEX_NS_MATH::Vec3::Zero;
	DUPLEX_NS_MATH::Vec3 normal = DUPLEX_NS_MATH::Vec3::Zero;
	DUPLEX_NS_MATH::Vec2 texCoords = DUPLEX_NS_MATH::Vec2::Zero;
	DUPLEX_NS_MATH::Vec3 tangent = DUPLEX_NS_MATH::Vec3::Zero;
	DUPLEX_NS_MATH::Vec3 bitTangent = DUPLEX_NS_MATH::Vec3::Zero;
};