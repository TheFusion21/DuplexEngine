#pragma once
#include "../math/vec3.h"
#include "../math/quaternion.h"
#include "../math/mat4x4.h"
#include "../namespaces.h"

namespace DUPLEX_NS_SIMD
{
	// Computes `count` world (TRS) matrices, one per (position, rotation, scale) triple:
	//   outWorldMatrices[i] = translate(positions[i]) * mat4_cast(rotations[i]) * scale(scales[i])
	// - exactly what every ECS system that builds a world matrix from a Transform already does
	// per-entity (see meshsystem.h/lightsystem.h). Automatically dispatches to an AVX2+FMA
	// implementation when GetCpuFeatures() (see cpufeatures.h) confirms the running CPU (and
	// OS) actually support it, falling back to a portable baseline otherwise - callers don't
	// need to know or care which ran.
	//
	// All four arrays must have at least `count` elements. Safe to call with count == 0.
	void ComputeWorldMatricesBatch(
		const DUPLEX_NS_MATH::Vec3* positions,
		const DUPLEX_NS_MATH::Quaternion* rotations,
		const DUPLEX_NS_MATH::Vec3* scales,
		DUPLEX_NS_MATH::Mat4x4* outWorldMatrices,
		unsigned int count);

	// Exposed only so tests can verify the AVX2 path produces the same results as the portable
	// one - production code should always go through ComputeWorldMatricesBatch above.
	namespace detail
	{
		void ComputeWorldMatricesBatchBaseline(
			const DUPLEX_NS_MATH::Vec3* positions,
			const DUPLEX_NS_MATH::Quaternion* rotations,
			const DUPLEX_NS_MATH::Vec3* scales,
			DUPLEX_NS_MATH::Mat4x4* outWorldMatrices,
			unsigned int count);

		void ComputeWorldMatricesBatchAVX2(
			const DUPLEX_NS_MATH::Vec3* positions,
			const DUPLEX_NS_MATH::Quaternion* rotations,
			const DUPLEX_NS_MATH::Vec3* scales,
			DUPLEX_NS_MATH::Mat4x4* outWorldMatrices,
			unsigned int count);
	}
}
