#pragma once

// CPUID stuff supported only on MSVC for now, irrelevant for non x86
#if defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))
#define PFC_HAVE_CPUID 1
namespace pfc {
	enum {
		CPU_HAVE_3DNOW		= 1 << 0,
		CPU_HAVE_3DNOW_EX	= 1 << 1,
		CPU_HAVE_SSE		= 1 << 2,
		CPU_HAVE_SSE2		= 1 << 3,
		CPU_HAVE_SSE3		= 1 << 4,
		CPU_HAVE_SSSE3		= 1 << 5,
		CPU_HAVE_SSE41		= 1 << 6,
		CPU_HAVE_SSE42		= 1 << 7,
	};

	bool query_cpu_feature_set(unsigned p_value);
};
#endif

#ifndef PFC_HAVE_CPUID
#define PFC_HAVE_CPUID 0
#endif