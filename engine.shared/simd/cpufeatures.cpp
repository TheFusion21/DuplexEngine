#include "cpufeatures.h"

// This file must compile and run correctly with zero special -march/-mavx/etc flags (it's part
// of the SSE2 baseline build) - it's the thing that decides, at runtime, whether it's safe to
// call into transformbatch_avx2.cpp's AVX2/FMA code at all. cpuid/xgetbv are plain baseline x86
// instructions (no special assembler target needed to encode them), so raw inline asm /
// unconditionally-available intrinsics are used here rather than compiler builtins that might
// be gated behind a target attribute.

#if defined(_M_X64) || defined(__x86_64__)
#define DUPLEX_SIMD_X86_64 1
#else
#define DUPLEX_SIMD_X86_64 0
#endif

#if DUPLEX_SIMD_X86_64
#if defined(_MSC_VER)
#include <intrin.h>
#else
#include <cpuid.h>
#endif
#endif

namespace
{
#if DUPLEX_SIMD_X86_64
	void CpuId(int leaf, int subleaf, unsigned int out[4])
	{
#if defined(_MSC_VER)
		int regs[4];
		__cpuidex(regs, leaf, subleaf);
		out[0] = static_cast<unsigned int>(regs[0]);
		out[1] = static_cast<unsigned int>(regs[1]);
		out[2] = static_cast<unsigned int>(regs[2]);
		out[3] = static_cast<unsigned int>(regs[3]);
#else
		unsigned int eax, ebx, ecx, edx;
		__get_cpuid_count(static_cast<unsigned int>(leaf), static_cast<unsigned int>(subleaf), &eax, &ebx, &ecx, &edx);
		out[0] = eax; out[1] = ebx; out[2] = ecx; out[3] = edx;
#endif
	}

	// XGETBV(XCR0) - confirms the OS has actually enabled saving/restoring the register state a
	// feature needs, not just that the CPU supports it (see CpuFeatures's comment).
	unsigned long long ReadXCR0()
	{
#if defined(_MSC_VER)
		return _xgetbv(0);
#else
		unsigned int eax, edx;
		__asm__ __volatile__("xgetbv" : "=a"(eax), "=d"(edx) : "c"(0));
		return (static_cast<unsigned long long>(edx) << 32) | eax;
#endif
	}
#endif // DUPLEX_SIMD_X86_64

	DUPLEX_NS_SIMD::CpuFeatures Detect()
	{
		DUPLEX_NS_SIMD::CpuFeatures features;
#if DUPLEX_SIMD_X86_64
		unsigned int regs[4];
		CpuId(0, 0, regs);
		unsigned int maxLeaf = regs[0];
		if (maxLeaf < 1)
			return features;

		CpuId(1, 0, regs);
		unsigned int ecx1 = regs[2];
		unsigned int edx1 = regs[3];

		features.sse2 = (edx1 & (1u << 26)) != 0;
		features.sse41 = (ecx1 & (1u << 19)) != 0;
		features.sse42 = (ecx1 & (1u << 20)) != 0;
		bool cpuHasAvx = (ecx1 & (1u << 28)) != 0;
		bool cpuHasFma = (ecx1 & (1u << 12)) != 0;
		bool osSavesExtendedState = (ecx1 & (1u << 27)) != 0; // OSXSAVE

		bool osSavesYmm = false;
		if (osSavesExtendedState)
		{
			unsigned long long xcr0 = ReadXCR0();
			// Bits 1 (SSE/XMM) and 2 (AVX/YMM) of XCR0 must both be set.
			osSavesYmm = (xcr0 & 0x6) == 0x6;
		}

		features.avx = cpuHasAvx && osSavesYmm;
		features.fma3 = cpuHasFma && osSavesYmm;

		if (maxLeaf >= 7)
		{
			CpuId(7, 0, regs);
			bool cpuHasAvx2 = (regs[1] & (1u << 5)) != 0; // EBX bit 5
			features.avx2 = cpuHasAvx2 && osSavesYmm;
		}
#endif // DUPLEX_SIMD_X86_64
		return features;
	}
}

const DUPLEX_NS_SIMD::CpuFeatures& DUPLEX_NS_SIMD::GetCpuFeatures()
{
	static const CpuFeatures features = Detect();
	return features;
}
