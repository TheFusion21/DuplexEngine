#pragma once

// EXTERNAL INCLUDES
#include <glm/glm.hpp>
// INTERNAL INCLUDES
#include "types.h"
#include "../namespaces.h"

namespace DUPLEX_NS_MATH
{
	using Vec2Int = glm::ivec2;

	inline const Vec2Int Vec2IntZero{ 0, 0 };
	inline const Vec2Int Vec2IntUnitX{ 1, 0 };
	inline const Vec2Int Vec2IntUnitY{ 0, 1 };
	inline const Vec2Int Vec2IntUnitScale{ 1, 1 };
}
