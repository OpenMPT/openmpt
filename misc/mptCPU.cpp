/*
 * mptCPU.cpp
 * ----------
 * Purpose: CPU feature detection.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "mptCPU.h"

#include "../common/mptStringBuffer.h"


OPENMPT_NAMESPACE_BEGIN


namespace CPU
{


#if defined(ENABLE_ASM)


uint32 RealProcSupport = 0;
uint32 ProcSupport = 0;
char ProcVendorID[16+1] = "";
char ProcBrandID[4*4*3+1] = "";
uint32 ProcRawCPUID = 0;
uint16 ProcFamily = 0;
uint8 ProcModel = 0;
uint8 ProcStepping = 0;


#if MPT_COMPILER_MSVC && (defined(ENABLE_X86) || defined(ENABLE_X64)) && defined(ENABLE_CPUID)


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
	std::string as_string4() const
	{
		std::string result;
		result.push_back(static_cast<uint8>((a >>  0) & 0xff));
		result.push_back(static_cast<uint8>((a >>  8) & 0xff));
		result.push_back(static_cast<uint8>((a >> 16) & 0xff));
		result.push_back(static_cast<uint8>((a >> 24) & 0xff));
		result.push_back(static_cast<uint8>((b >>  0) & 0xff));
		result.push_back(static_cast<uint8>((b >>  8) & 0xff));
		result.push_back(static_cast<uint8>((b >> 16) & 0xff));
		result.push_back(static_cast<uint8>((b >> 24) & 0xff));
		result.push_back(static_cast<uint8>((c >>  0) & 0xff));
		result.push_back(static_cast<uint8>((c >>  8) & 0xff));
		result.push_back(static_cast<uint8>((c >> 16) & 0xff));
		result.push_back(static_cast<uint8>((c >> 24) & 0xff));
		result.push_back(static_cast<uint8>((d >>  0) & 0xff));
		result.push_back(static_cast<uint8>((d >>  8) & 0xff));
		result.push_back(static_cast<uint8>((d >> 16) & 0xff));
		result.push_back(static_cast<uint8>((d >> 24) & 0xff));
		return result;
	}
};


static cpuid_result cpuid(uint32 function)
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


void Init()
{

	RealProcSupport = 0;
	ProcSupport = 0;
	mpt::String::WriteAutoBuf(ProcVendorID) = "";
	mpt::String::WriteAutoBuf(ProcBrandID) = "";
	ProcRawCPUID = 0;
	ProcFamily = 0;
	ProcModel = 0;
	ProcStepping = 0;

	ProcSupport |= feature::asm_intrinsics;
	ProcSupport |= feature::cpuid;

	{

		cpuid_result VendorString = cpuid(0x00000000u);
		mpt::String::WriteAutoBuf(ProcVendorID) = VendorString.as_string();
		if(VendorString.a >= 0x00000001u)
		{
			cpuid_result StandardFeatureFlags = cpuid(0x00000001u);
			ProcRawCPUID = StandardFeatureFlags.a;
			uint32 Stepping   = (StandardFeatureFlags.a >>  0) & 0x0f;
			uint32 BaseModel  = (StandardFeatureFlags.a >>  4) & 0x0f;
			uint32 BaseFamily = (StandardFeatureFlags.a >>  8) & 0x0f;
			uint32 ExtModel   = (StandardFeatureFlags.a >> 16) & 0x0f;
			uint32 ExtFamily  = (StandardFeatureFlags.a >> 20) & 0xff;
			if(BaseFamily == 0xf)
			{
				ProcFamily = static_cast<uint16>(ExtFamily + BaseFamily);
			} else
			{
				ProcFamily = static_cast<uint16>(BaseFamily);
			}
			if((BaseFamily == 0x6) || (BaseFamily == 0xf))
			{
				ProcModel = static_cast<uint8>((ExtModel << 4) | (BaseModel << 0));
			} else
			{
				ProcModel = static_cast<uint8>(BaseModel);
			}
			ProcStepping = static_cast<uint8>(Stepping);
			if(StandardFeatureFlags.d & (1<<23)) ProcSupport |= feature::mmx;
			if(StandardFeatureFlags.d & (1<<25)) ProcSupport |= feature::sse;
			if(StandardFeatureFlags.d & (1<<26)) ProcSupport |= feature::sse2;
			if(StandardFeatureFlags.c & (1<< 0)) ProcSupport |= feature::sse3;
			if(StandardFeatureFlags.c & (1<< 9)) ProcSupport |= feature::ssse3;
			if(StandardFeatureFlags.c & (1<<19)) ProcSupport |= feature::sse4_1;
			if(StandardFeatureFlags.c & (1<<20)) ProcSupport |= feature::sse4_2;
			if(StandardFeatureFlags.c & (1<<28)) ProcSupport |= feature::avx;
		}

		cpuid_result ExtendedVendorString = cpuid(0x80000000u);
		if(ExtendedVendorString.a >= 0x80000001u)
		{
			cpuid_result ExtendedFeatureFlags = cpuid(0x80000001u);
			if(ExtendedFeatureFlags.d & (1<<29)) ProcSupport |= feature::lm;
		}
		if(ExtendedVendorString.a >= 0x80000004u)
		{
			mpt::String::WriteAutoBuf(ProcBrandID) = cpuid(0x80000002u).as_string4() + cpuid(0x80000003u).as_string4() + cpuid(0x80000004u).as_string4();
			if(ExtendedVendorString.a >= 0x80000007u)
			{
				cpuid_result ExtendedFeatures = cpuid(0x80000007u);
				if(ExtendedFeatures.b & (1<< 5)) ProcSupport |= feature::avx2;
			}
		}

	}

	RealProcSupport = ProcSupport;

}


#elif MPT_COMPILER_MSVC && (defined(ENABLE_X86) || defined(ENABLE_X64))


void Init()
{

	RealProcSupport = 0;
	ProcSupport = 0;
	mpt::String::WriteAutoBuf(ProcVendorID) = "";
	mpt::String::WriteAutoBuf(ProcBrandID) = "";
	ProcRawCPUID = 0;
	ProcFamily = 0;
	ProcModel = 0;
	ProcStepping = 0;

	ProcSupport |= feature::asm_intrinics;

	{

		if(IsProcessorFeaturePresent(PF_MMX_INSTRUCTIONS_AVAILABLE) != 0)    ProcSupport |= feature::mmx;
		if(IsProcessorFeaturePresent(PF_XMMI_INSTRUCTIONS_AVAILABLE) != 0)   ProcSupport |= feature::sse;
		if(IsProcessorFeaturePresent(PF_XMMI64_INSTRUCTIONS_AVAILABLE) != 0) ProcSupport |= feature::sse2;
		if(IsProcessorFeaturePresent(PF_SSE3_INSTRUCTIONS_AVAILABLE) != 0)   ProcSupport |= feature::sse3;

	}

	RealProcSupport = ProcSupport;

}


#else // !( MPT_COMPILER_MSVC && ENABLE_X86 )


void Init()
{
	ProcSupport = 0;
}


#endif // MPT_COMPILER_MSVC && ENABLE_X86

#endif // ENABLE_ASM


#ifdef MODPLUG_TRACKER


uint32 GetMinimumFeatures()
{
	uint32 flags = 0;
	#ifdef ENABLE_ASM
		#if MPT_COMPILER_MSVC
			#if defined(_M_X64)
				flags |= featureset::amd64;
			#elif defined(_M_IX86)
				#if defined(_M_IX86_FP)
					#if (_M_IX86_FP >= 2)
						flags |= featureset::x86_sse2;
					#elif (_M_IX86_FP == 1)
						flags |= featureset::x86_sse;
					#endif
				#else
					flags |= featureset::x86_i586;
				#endif
			#endif
		#endif	
	#endif // ENABLE_ASM
	return flags;
}



int GetMinimumSSEVersion()
{
	int minimumSSEVersion = 0;
	#if MPT_COMPILER_MSVC
		#if defined(_M_X64)
			minimumSSEVersion = 2;
		#elif defined(_M_IX86)
			#if defined(_M_IX86_FP)
				#if (_M_IX86_FP >= 2)
					minimumSSEVersion = 2;
				#elif (_M_IX86_FP == 1)
					minimumSSEVersion = 1;
				#endif
			#endif
		#endif
	#endif
	return minimumSSEVersion;
}


int GetMinimumAVXVersion()
{
	int minimumAVXVersion = 0;
	#if MPT_COMPILER_MSVC
		#if defined(_M_IX86_FP)
			#if defined(__AVX2__)
				minimumAVXVersion = 2;
			#elif defined(__AVX__)
				minimumAVXVersion = 1;
			#endif
		#endif
	#endif
	return minimumAVXVersion;
}


#endif


#if !defined(MODPLUG_TRACKER) && !defined(ENABLE_ASM)

MPT_MSVC_WORKAROUND_LNK4221(mptCPU)

#endif


} // namespace CPU


OPENMPT_NAMESPACE_END
