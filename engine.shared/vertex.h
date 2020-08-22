#pragma once

// EXTERNAL INCLUDES
// INTERNAL INCLUDES
#include "math/vec3.h"
#include "math/vec2.h"
#include "utils/color.h"
#include "math/quaternion.h"
struct Vertex
{
	Engine::Math::Vec3 position;
	Engine::Math::Vec3 normal;
	Engine::Math::Vec2 texCoords;
	Engine::Math::Vec3 tangent;
	Engine::Math::Vec3 bitTangent;
};