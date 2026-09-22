// Compiled with -mavx2 -mfma (GCC/Clang) or /arch:AVX2 (MSVC) - see the target_source_files
// override in engine.shared/CMakeLists.txt. This is the ONLY translation unit in the binary
// allowed to contain AVX2/FMA instructions; ComputeWorldMatricesBatch (transformbatch_baseline.
// cpp) only calls into ComputeWorldMatricesBatchAVX2 after CpuFeatures confirms both the CPU
// and OS actually support AVX2+FMA (see cpufeatures.cpp) - never unconditionally.
#include "transformbatch.h"
#include <immintrin.h>

using namespace DUPLEX_NS_MATH;

namespace
{
	constexpr unsigned int kLanes = 8;

	// Processes exactly 8 (position, rotation, scale) triples at once. The quaternion-to-
	// rotation-matrix terms (the actual multiply-heavy part of this operation) are computed
	// 8-wide via AVX2/FMA; the final TRS scatter into each output glm::mat4's column-major
	// memory layout is done per-lane (glm::mat4 is only 3 columns x 4 rows in a plain array -
	// not worth a hand-rolled SIMD transpose for that part, and doing it scalar keeps this
	// numerically easy to verify against the baseline path element-by-element).
	void ComputeEight(
		const Vec3* positions, const Quaternion* rotations, const Vec3* scales,
		Mat4x4* outWorldMatrices)
	{
		alignas(32) float qx[kLanes], qy[kLanes], qz[kLanes], qw[kLanes];
		for (unsigned int lane = 0; lane < kLanes; lane++)
		{
			qx[lane] = rotations[lane].x;
			qy[lane] = rotations[lane].y;
			qz[lane] = rotations[lane].z;
			qw[lane] = rotations[lane].w;
		}

		__m256 x = _mm256_load_ps(qx);
		__m256 y = _mm256_load_ps(qy);
		__m256 z = _mm256_load_ps(qz);
		__m256 w = _mm256_load_ps(qw);

		__m256 tx = _mm256_add_ps(x, x);
		__m256 ty = _mm256_add_ps(y, y);
		__m256 tz = _mm256_add_ps(z, z);

		__m256 twx = _mm256_mul_ps(w, tx);
		__m256 twy = _mm256_mul_ps(w, ty);
		__m256 twz = _mm256_mul_ps(w, tz);

		__m256 txx = _mm256_mul_ps(x, tx);
		__m256 txy = _mm256_mul_ps(x, ty);
		__m256 txz = _mm256_mul_ps(x, tz);

		__m256 tyy = _mm256_mul_ps(y, ty);
		__m256 tyz = _mm256_mul_ps(y, tz);

		__m256 tzz = _mm256_mul_ps(z, tz);

		__m256 one = _mm256_set1_ps(1.0f);

		// Same formula as glm::mat4_cast / the old hand-rolled Mat4x4::FromOrientation -
		// verified bit-for-bit equivalent to glm::mat4_cast (after the row/column-major
		// reconciliation) during the GLM migration. r_ij: row i, column j of the 3x3 rotation.
		__m256 r00 = _mm256_sub_ps(one, _mm256_add_ps(tyy, tzz));
		__m256 r01 = _mm256_sub_ps(txy, twz);
		__m256 r02 = _mm256_add_ps(txz, twy);

		__m256 r10 = _mm256_add_ps(txy, twz);
		__m256 r11 = _mm256_sub_ps(one, _mm256_add_ps(txx, tzz));
		__m256 r12 = _mm256_sub_ps(tyz, twx);

		__m256 r20 = _mm256_sub_ps(txz, twy);
		__m256 r21 = _mm256_add_ps(tyz, twx);
		__m256 r22 = _mm256_sub_ps(one, _mm256_add_ps(txx, tyy));

		alignas(32) float m00[kLanes], m01[kLanes], m02[kLanes];
		alignas(32) float m10[kLanes], m11[kLanes], m12[kLanes];
		alignas(32) float m20[kLanes], m21[kLanes], m22[kLanes];
		_mm256_store_ps(m00, r00); _mm256_store_ps(m01, r01); _mm256_store_ps(m02, r02);
		_mm256_store_ps(m10, r10); _mm256_store_ps(m11, r11); _mm256_store_ps(m12, r12);
		_mm256_store_ps(m20, r20); _mm256_store_ps(m21, r21); _mm256_store_ps(m22, r22);

		for (unsigned int lane = 0; lane < kLanes; lane++)
		{
			const Vec3& s = scales[lane];
			const Vec3& p = positions[lane];
			Mat4x4& out = outWorldMatrices[lane];

			// glm::mat4 is column-major (out[col][row]); translate(p) * R * scale(s) scales
			// each rotation column j by scales[j] and places p in the translation column -
			// same composition verified against the old math library and glm's own operators
			// in ComputeWorldMatricesBatchBaseline.
			out[0][0] = m00[lane] * s.x; out[0][1] = m10[lane] * s.x; out[0][2] = m20[lane] * s.x; out[0][3] = 0.0f;
			out[1][0] = m01[lane] * s.y; out[1][1] = m11[lane] * s.y; out[1][2] = m21[lane] * s.y; out[1][3] = 0.0f;
			out[2][0] = m02[lane] * s.z; out[2][1] = m12[lane] * s.z; out[2][2] = m22[lane] * s.z; out[2][3] = 0.0f;
			out[3][0] = p.x; out[3][1] = p.y; out[3][2] = p.z; out[3][3] = 1.0f;
		}
	}
}

void DUPLEX_NS_SIMD::detail::ComputeWorldMatricesBatchAVX2(
	const Vec3* positions, const Quaternion* rotations, const Vec3* scales,
	Mat4x4* outWorldMatrices, unsigned int count)
{
	unsigned int processed = 0;
	for (; processed + kLanes <= count; processed += kLanes)
	{
		ComputeEight(positions + processed, rotations + processed, scales + processed, outWorldMatrices + processed);
	}

	// Tail (count % 8 leftover entries): not worth a masked/partial AVX2 path for what's at
	// most 7 entries - the portable baseline is exact and simple.
	unsigned int remaining = count - processed;
	if (remaining > 0)
	{
		ComputeWorldMatricesBatchBaseline(positions + processed, rotations + processed, scales + processed, outWorldMatrices + processed, remaining);
	}
}
