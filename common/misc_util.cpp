/*
 * misc_util.cpp
 * -------------
 * Purpose: Various useful utility functions.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "misc_util.h"


#if defined(ENABLE_ASM)


uint32 ProcSupport = 0;


#if MPT_COMPILER_MSVC && defined(ENABLE_X86)


#include <intrin.h>


typedef char cpuid_result_string[12];


struct cpuid_result {
	uint32 a;
	uint32 b;
	uint32 c;
	uint32 d;
	std::string as_string() const
	{
		cpuid_result_string result;
		result[0+0] = (b >> 0) & 0xff;
		result[0+1] = (b >> 8) & 0xff;
		result[0+2] = (b >>16) & 0xff;
		result[0+3] = (b >>24) & 0xff;
		result[4+0] = (d >> 0) & 0xff;
		result[4+1] = (d >> 8) & 0xff;
		result[4+2] = (d >>16) & 0xff;
		result[4+3] = (d >>24) & 0xff;
		result[8+0] = (c >> 0) & 0xff;
		result[8+1] = (c >> 8) & 0xff;
		result[8+2] = (c >>16) & 0xff;
		result[8+3] = (c >>24) & 0xff;
		return std::string(result, result + 12);
	}
};


static cpuid_result cpuid(uint32 function)
//----------------------------------------
{
	cpuid_result result;
	int CPUInfo[4];
	__cpuid(CPUInfo, function);
	result.a = CPUInfo[0];
	result.b = CPUInfo[1];
	result.c = CPUInfo[2];
	result.d = CPUInfo[3];
	return result;
}


static bool has_cpuid()
//---------------------
{
	const uint32 eflags_cpuid = 1<<21;
	uint32 old_eflags = __readeflags();
	__writeeflags(old_eflags ^ eflags_cpuid);
	bool result = ((__readeflags() ^ old_eflags) & eflags_cpuid) ? true : false;
	__writeeflags(old_eflags);
	return result;
}


void InitProcSupport()
//--------------------
{

	ProcSupport = 0;

	if(has_cpuid())
	{

		cpuid_result VendorString = cpuid(0x00000000u);

		cpuid_result StandardFeatureFlags = cpuid(0x00000001u);
		if(StandardFeatureFlags.d & (1<<23)) ProcSupport |= PROCSUPPORT_MMX;
		if(StandardFeatureFlags.d & (1<<25)) ProcSupport |= PROCSUPPORT_SSE;
		if(StandardFeatureFlags.d & (1<<26)) ProcSupport |= PROCSUPPORT_SSE2;
		if(StandardFeatureFlags.c & (1<< 0)) ProcSupport |= PROCSUPPORT_SSE3;

		if(VendorString.as_string() == "AuthenticAMD")
		{

			cpuid_result ExtendedVendorString = cpuid(0x80000000u);
			if(ExtendedVendorString.a >= 0x80000001u)
			{

				cpuid_result ExtendedFeatureFlags = cpuid(0x80000001u);
				if(ExtendedFeatureFlags.d & (1<<22)) ProcSupport |= PROCSUPPORT_AMD_MMXEXT;
				if(ExtendedFeatureFlags.d & (1<<31)) ProcSupport |= PROCSUPPORT_AMD_3DNOW;
				if(ExtendedFeatureFlags.d & (1<<30)) ProcSupport |= PROCSUPPORT_AMD_3DNOW2;

			}

		}

	}

}


#else // !( MPT_COMPILER_MSVC && ENABLE_X86 )


void InitProcSupport()
//--------------------
{
	ProcSupport = 0;
}


#endif // MPT_COMPILER_MSVC && ENABLE_X86


#endif // ENABLE_ASM

