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


#if MPT_COMPILER_MSVC && (defined(ENABLE_X86) || defined(ENABLE_AMD64)) && defined(ENABLE_CPUID)


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


static cpuid_result cpuidex(uint32 function_a, uint32 function_c)
{
	cpuid_result result;
	int CPUInfo[4];
	__cpuidex(CPUInfo, function_a, function_c);
	result.a = CPUInfo[0];
	result.b = CPUInfo[1];
	result.c = CPUInfo[2];
	result.d = CPUInfo[3];
	return result;
}


Info::Info()
{

	cpuid_result VendorString = cpuid(0x00000000u);
	mpt::String::WriteAutoBuf(VendorID) = VendorString.as_string();
	if(VendorString.a >= 0x00000001u)
	{
		cpuid_result StandardFeatureFlags = cpuid(0x00000001u);
		CPUID = StandardFeatureFlags.a;
		uint32 BaseStepping = (StandardFeatureFlags.a >>  0) & 0x0f;
		uint32 BaseModel    = (StandardFeatureFlags.a >>  4) & 0x0f;
		uint32 BaseFamily   = (StandardFeatureFlags.a >>  8) & 0x0f;
		uint32 ExtModel     = (StandardFeatureFlags.a >> 16) & 0x0f;
		uint32 ExtFamily    = (StandardFeatureFlags.a >> 20) & 0xff;
		if(BaseFamily == 0xf)
		{
			Family = static_cast<uint16>(ExtFamily + BaseFamily);
		} else
		{
			Family = static_cast<uint16>(BaseFamily);
		}
		if((BaseFamily == 0x6) || (BaseFamily == 0xf))
		{
			Model = static_cast<uint8>((ExtModel << 4) | (BaseModel << 0));
		} else
		{
			Model = static_cast<uint8>(BaseModel);
		}
		Stepping = static_cast<uint8>(BaseStepping);
		if(StandardFeatureFlags.d & (1<<23)) AvailableFeatures |= feature::mmx;
		if(StandardFeatureFlags.d & (1<<25)) AvailableFeatures |= feature::sse;
		if(StandardFeatureFlags.d & (1<<26)) AvailableFeatures |= feature::sse2;
		if(StandardFeatureFlags.c & (1<< 0)) AvailableFeatures |= feature::sse3;
		if(StandardFeatureFlags.c & (1<< 9)) AvailableFeatures |= feature::ssse3;
		if(StandardFeatureFlags.c & (1<<19)) AvailableFeatures |= feature::sse4_1;
		if(StandardFeatureFlags.c & (1<<20)) AvailableFeatures |= feature::sse4_2;
		if(StandardFeatureFlags.c & (1<<28)) AvailableFeatures |= feature::avx;
	}
	if(VendorString.a >= 0x00000007u)
	{
		cpuid_result ExtendedFeatures = cpuidex(0x00000007u, 0x00000000u);
		if(ExtendedFeatures.b & (1<< 5)) AvailableFeatures |= feature::avx2;
	}

	cpuid_result ExtendedVendorString = cpuid(0x80000000u);
	if(ExtendedVendorString.a >= 0x80000001u)
	{
		cpuid_result ExtendedFeatureFlags = cpuid(0x80000001u);
		if(ExtendedFeatureFlags.d & (1<<29)) AvailableFeatures |= feature::lm;
	}
	if(ExtendedVendorString.a >= 0x80000004u)
	{
		mpt::String::WriteAutoBuf(BrandID) = cpuid(0x80000002u).as_string4() + cpuid(0x80000003u).as_string4() + cpuid(0x80000004u).as_string4();
	}

}


#elif MPT_COMPILER_MSVC && (defined(ENABLE_X86) || defined(ENABLE_AMD64))


Info::Info()
{

	if(IsProcessorFeaturePresent(PF_MMX_INSTRUCTIONS_AVAILABLE) != 0)    AvailableFeatures |= feature::mmx;
	if(IsProcessorFeaturePresent(PF_XMMI_INSTRUCTIONS_AVAILABLE) != 0)   AvailableFeatures |= feature::sse;
	if(IsProcessorFeaturePresent(PF_XMMI64_INSTRUCTIONS_AVAILABLE) != 0) AvailableFeatures |= feature::sse2;
	if(IsProcessorFeaturePresent(PF_SSE3_INSTRUCTIONS_AVAILABLE) != 0)   AvailableFeatures |= feature::sse3;

}


#else // !( MPT_COMPILER_MSVC && ENABLE_X86 )


Info::Info()
{
	return;
}


#endif // MPT_COMPILER_MSVC && ENABLE_X86


const Info & Info::Get()
{
	static Info info;
	return info;
}


struct InfoInitializer
{
	InfoInitializer()
	{
		Info::Get();
	}
};


static InfoInitializer g_InfoInitializer;


void EnableAvailableFeatures()
{
	EnabledFeatures = Info::Get().AvailableFeatures;
}


#endif // ENABLE_ASM


uint32 GetMinimumFeatures()
{
	uint32 flags = 0;
	#ifdef ENABLE_ASM
		#if MPT_COMPILER_MSVC
			#if defined(_M_X64)
				flags |= feature::lm | feature::sse | feature::sse2;
			#elif defined(_M_IX86)
				#if defined(_M_IX86_FP)
					#if (_M_IX86_FP >= 2)
						flags |= feature::sse | feature::sse2;
					#elif (_M_IX86_FP == 1)
						flags |= feature::sse;
					#endif
				#endif
			#endif
			#if defined(__AVX__)
				flags |= feature::avx;
			#endif
			#if defined(__AVX2__)
				flags |= feature::avx2;
			#endif
		#endif	
	#endif // ENABLE_ASM
	return flags;
}



} // namespace CPU


OPENMPT_NAMESPACE_END
