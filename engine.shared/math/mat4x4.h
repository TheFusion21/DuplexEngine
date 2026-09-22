#pragma once

// EXTERNAL INCLUDES
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
// INTERNAL INCLUDES
#include "types.h"
#include "../namespaces.h"

namespace DUPLEX_NS_MATH
{
	using Mat4x4 = glm::mat4;

	inline const Mat4x4 Mat4x4Identity{ 1.0f };

	// GLM's mat4 is stored column-major in memory; the shaders' modelConstant.world /
	// worldConstant.viewProj / Light.transform cbuffer fields are row_major (verified via
	// spirv-cross reflection on the compiled shader during the GLM migration). glm::transpose()
	// reconciles the two so the raw bytes uploaded to the GPU are unchanged from before the
	// migration (verified numerically against the old hand-rolled Mat4x4 for representative
	// TRS/view/perspective matrices) - apply this at the exact point a matrix is written into a
	// shader-facing struct (driver.graphics/shadercb.h), not when it's composed/multiplied.
	inline Mat4x4 ToShaderLayout(const Mat4x4& m)
	{
		return glm::transpose(m);
	}
}
