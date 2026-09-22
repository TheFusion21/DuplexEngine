#include "transformbatch.h"
#include "cpufeatures.h"

using namespace DUPLEX_NS_MATH;

void DUPLEX_NS_SIMD::detail::ComputeWorldMatricesBatchBaseline(
	const Vec3* positions, const Quaternion* rotations, const Vec3* scales,
	Mat4x4* outWorldMatrices, unsigned int count)
{
	for (unsigned int i = 0; i < count; i++)
	{
		outWorldMatrices[i] = glm::translate(Mat4x4(1.0f), positions[i])
			* glm::mat4_cast(rotations[i])
			* glm::scale(Mat4x4(1.0f), scales[i]);
	}
}

void DUPLEX_NS_SIMD::ComputeWorldMatricesBatch(
	const Vec3* positions, const Quaternion* rotations, const Vec3* scales,
	Mat4x4* outWorldMatrices, unsigned int count)
{
	const CpuFeatures& cpu = GetCpuFeatures();
	if (cpu.avx2 && cpu.fma3)
	{
		detail::ComputeWorldMatricesBatchAVX2(positions, rotations, scales, outWorldMatrices, count);
	}
	else
	{
		detail::ComputeWorldMatricesBatchBaseline(positions, rotations, scales, outWorldMatrices, count);
	}
}
