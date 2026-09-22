#pragma once

// EXTERNAL INCLUDES
#include <glm/glm.hpp>
// INTERNAL INCLUDES
#include "types.h"
#include "../namespaces.h"

namespace DUPLEX_NS_MATH
{
	using Vec2 = glm::vec2;

	inline const Vec2 Vec2Zero{ 0.0f, 0.0f };
	inline const Vec2 Vec2UnitX{ 1.0f, 0.0f };
	inline const Vec2 Vec2UnitY{ 0.0f, 1.0f };
	inline const Vec2 Vec2UnitScale{ 1.0f, 1.0f };
}
