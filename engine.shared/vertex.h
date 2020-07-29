#pragma once

// EXTERNAL INCLUDES
// INTERNAL INCLUDES
#include "math/vec3.h"
#include "math/vec2.h"
#include "utils/color.h"

struct Vertex
{
	Engine::Math::Vec3 position;
	Engine::Utils::FloatColor color;
	Engine::Math::Vec3 normal;
	Engine::Math::Vec2 texCoords;
};