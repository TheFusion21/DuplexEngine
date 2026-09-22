#pragma once
// EXTERNAL INCLUDES
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
// INTERNAL INCLUDES
#include "types.h"
#include "vec3.h"
#include "../namespaces.h"

namespace DUPLEX_NS_MATH
{
	using Quaternion = glm::quat;

	// glm::quat's component order is (w,x,y,z); this is the identity rotation, matching
	// glm::quat's own default-constructed value. Transform's default rotation used to be the
	// hand-rolled Quaternion::Zero == (0,0,0,0), a degenerate non-unit quaternion - any entity
	// that never explicitly set its rotation would have collapsed FromOrientation() to a
	// zero/garbage matrix. Fixed as part of this migration; see transform.h.
	inline const Quaternion QuatIdentity{ 1.0f, 0.0f, 0.0f, 0.0f };

	// No direct GLM equivalent: composes Z, then Y, then X angle-axis rotations (degrees),
	// matching the engine's existing Euler convention exactly (verified against the old
	// hand-rolled Quaternion::FromEuler during the GLM migration).
	inline Quaternion QuaternionFromEuler(Vec3 eulerDegrees)
	{
		return glm::angleAxis(glm::radians(eulerDegrees.z), Vec3UnitZ)
			 * glm::angleAxis(glm::radians(eulerDegrees.y), Vec3UnitY)
			 * glm::angleAxis(glm::radians(eulerDegrees.x), Vec3UnitX);
	}
}
