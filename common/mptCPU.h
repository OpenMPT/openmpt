/*
 * mptCPU.h
 * --------
 * Purpose: CPU feature detection.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once


OPENMPT_NAMESPACE_BEGIN


#ifdef ENABLE_ASM
#define PROCSUPPORT_MMX        0x00001 // Processor supports MMX instructions
#define PROCSUPPORT_SSE        0x00010 // Processor supports SSE instructions
#define PROCSUPPORT_SSE2       0x00020 // Processor supports SSE2 instructions
#define PROCSUPPORT_SSE3       0x00040 // Processor supports SSE3 instructions
#define PROCSUPPORT_AMD_MMXEXT 0x10000 // Processor supports AMD MMX extensions
#define PROCSUPPORT_AMD_3DNOW  0x20000 // Processor supports AMD 3DNow! instructions
#define PROCSUPPORT_AMD_3DNOW2 0x40000 // Processor supports AMD 3DNow!2 instructions
extern uint32 ProcSupport;
void InitProcSupport();
static inline uint32 GetProcSupport()
{
	return ProcSupport;
}
#endif // ENABLE_ASM


OPENMPT_NAMESPACE_END
