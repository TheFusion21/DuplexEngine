#pragma once
#include "../namespaces.h"

namespace DUPLEX_NS_SIMD
{
	// Runtime-detected x86-64 SIMD ISA support, queried once (see GetCpuFeatures()) and cached.
	// AVX/AVX2/FMA3 are only reported true if the OS has also enabled XSAVE for the YMM
	// register state (checked via XGETBV(XCR0), not just the CPUID feature bit) - a CPU can
	// support AVX in hardware while an old/misconfigured OS hasn't enabled saving it, in which
	// case using AVX instructions would still fault.
	struct CpuFeatures
	{
		bool sse2 = false;
		bool sse41 = false;
		bool sse42 = false;
		bool avx = false;
		bool avx2 = false;
		bool fma3 = false;
	};

	// Detects CPU features on first call; every call after that returns the same cached result.
	const CpuFeatures& GetCpuFeatures();
}
