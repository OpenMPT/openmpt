/*
 * mptCPU.h
 * --------
 * Purpose: CPU feature detection.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "openmpt/all/BuildSettings.hpp"


OPENMPT_NAMESPACE_BEGIN


namespace CPU
{


namespace feature {
inline constexpr uint32 lm             = 0x00004; // Processor supports long mode (amd64)
inline constexpr uint32 mmx            = 0x00010; // Processor supports MMX instructions
inline constexpr uint32 sse            = 0x00100; // Processor supports SSE instructions
inline constexpr uint32 sse2           = 0x00200; // Processor supports SSE2 instructions
inline constexpr uint32 sse3           = 0x00400; // Processor supports SSE3 instructions
inline constexpr uint32 ssse3          = 0x00800; // Processor supports SSSE3 instructions
inline constexpr uint32 sse4_1         = 0x01000; // Processor supports SSE4.1 instructions
inline constexpr uint32 sse4_2         = 0x02000; // Processor supports SSE4.2 instructions
inline constexpr uint32 avx            = 0x10000; // Processor supports AVX instructions
inline constexpr uint32 avx2           = 0x20000; // Processor supports AVX2 instructions
} // namespace feature


#ifdef ENABLE_ASM


inline uint32 EnabledFeatures = 0;


struct Info
{
public:
	uint32 AvailableFeatures = 0;
	uint32 CPUID = 0;
	char VendorID[16+1] = {};
	char BrandID[4*4*3+1] = {};
	uint16 Family = 0;
	uint8 Model = 0;
	uint8 Stepping = 0;
private:
	Info();
public:
	static const Info & Get();
};


void EnableAvailableFeatures();


struct AvailableFeaturesEnabler
{
	AvailableFeaturesEnabler()
	{
		EnableAvailableFeatures();
	}
};


// enabled processor features for inline asm and intrinsics
MPT_FORCEINLINE uint32 GetEnabledFeatures()
{
	return EnabledFeatures;
}

MPT_FORCEINLINE bool HasFeatureSet(uint32 features)
{
	return features == (GetEnabledFeatures() & features);
}


#endif // ENABLE_ASM


uint32 GetMinimumFeatures();


} // namespace CPU


OPENMPT_NAMESPACE_END
