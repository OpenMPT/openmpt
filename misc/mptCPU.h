/*
 * mptCPU.h
 * --------
 * Purpose: CPU feature detection.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "BuildSettings.h"


OPENMPT_NAMESPACE_BEGIN


namespace CPU
{


#ifdef MODPLUG_TRACKER

namespace feature {
inline constexpr uint32 asm_intrinsics = 0x00001; // assembly and intrinsics are enabled at runtime
inline constexpr uint32 cpuid          = 0x00002; // Processor supports modern cpuid
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

namespace featureset {
inline constexpr uint32 x86_i586 = 0u | feature::cpuid                                             ;
inline constexpr uint32 x86_sse  = 0u | feature::cpuid | feature::sse                              ;
inline constexpr uint32 x86_sse2 = 0u | feature::cpuid | feature::sse | feature::sse2              ;
inline constexpr uint32 amd64    = 0u | feature::cpuid | feature::sse | feature::sse2 | feature::lm;
} // namespace featureset

#endif


#ifdef ENABLE_ASM

extern uint32 RealProcSupport;
extern uint32 ProcSupport;
extern char ProcVendorID[16+1];
extern char ProcBrandID[4*4*3+1];
extern uint32 ProcRawCPUID;
extern uint16 ProcFamily;
extern uint8 ProcModel;
extern uint8 ProcStepping;

void Init();

// enabled processor features for inline asm and intrinsics
MPT_FORCEINLINE uint32 GetEnabledFeatures()
{
	return ProcSupport;
}

// available processor features
MPT_FORCEINLINE uint32 GetAvailableFeatures()
{
	return RealProcSupport;
}

MPT_FORCEINLINE bool HasFeatureSet(uint32 features)
{
	return features == (GetEnabledFeatures() & features);
}

#endif // ENABLE_ASM


#ifdef MODPLUG_TRACKER
uint32 GetMinimumFeatures();
int GetMinimumSSEVersion();
int GetMinimumAVXVersion();
#endif


} // namespace CPU


OPENMPT_NAMESPACE_END
