#pragma once

// EXTERNAL INCLUDES
#include <glm/glm.hpp>
// INTERNAL INCLUDES
#include "types.h"
#include "../namespaces.h"

namespace DUPLEX_NS_MATH
{
	using Vec3 = glm::vec3;

	inline const Vec3 Vec3Zero{ 0.0f, 0.0f, 0.0f };
	inline const Vec3 Vec3UnitX{ 1.0f, 0.0f, 0.0f };
	inline const Vec3 Vec3UnitY{ 0.0f, 1.0f, 0.0f };
	inline const Vec3 Vec3UnitZ{ 0.0f, 0.0f, 1.0f };
	inline const Vec3 Vec3UnitScale{ 1.0f, 1.0f, 1.0f };
}
