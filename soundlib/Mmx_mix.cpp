/*
 * This program is  free software; you can redistribute it  and modify it
 * under the terms of the GNU  General Public License as published by the
 * Free Software Foundation; either version 2  of the license or (at your
 * option) any later version.
 *
 * Authors: Olivier Lapicque <olivierl@jps.net>
 * Name                 Date             Description
 * Olivier Lapicque     --/--/--         Creation
 * Trevor Nunes         26/01/04         encapsulated MMX,AMD,SSE with #define flags
 *                                       moved X86_xxx functions to end of file.
*/

////////////////////////////////////////////////////////////////////////
//
// x86 ( AMD/INTEL ) based low level based mixing functions:
// This file contains critical code. The basic X86 functions are
// defined at the bottom of the file. #define's are used to isolate
// the different flavours of functionality:
// ENABLE_MMX, ENABLE_AMDNOW, ENABLE_SSE flags must be set to
// to compile the optimized sections of the code. In both cases the 
// X86_xxxxxx functions will compile. 
//
////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "sndfile.h"

extern short int gFastSinc[];
extern short int gKaiserSinc[];
extern short int gDownsample13x[];
extern short int gDownsample2x[];

#define PROCSUPPORT_CPUID	0x01
#define PROCSUPPORT_MMX		0x02
#define PROCSUPPORT_MMXEX	0x04
#define PROCSUPPORT_3DNOW	0x08
#define PROCSUPPORT_SSE		0x10


#define CHNOFS_PCURRENTSAMPLE	0
#define CHNOFS_NPOS				4
#define CHNOFS_NPOSLO			8
#define CHNOFS_NINC				12
#define CHNOFS_NRIGHTVOL		16
#define CHNOFS_NLEFTVOL			20
#define CHNOFS_NRIGHTRAMP		24
#define CHNOFS_NLEFTRAMP		28


#ifdef ENABLE_MMX

#define MMX_PARAM1				20[esp]
#define MMX_PARAM2				24[esp]
#define MMX_PARAM3				28[esp]
#define MMX_PARAM4				32[esp]
#define MMX_PARAM5				36[esp]

#define MMX_PCHANNEL			MMX_PARAM1
#define MMX_PBUFFER				MMX_PARAM2
#define MMX_PBUFMAX				MMX_PARAM3

#define MMX_ENTER	\
	__asm push ebx	\
	__asm push esi	\
	__asm push edi	\
	__asm push ebp


#define MMX_LEAVE	\
	__asm pop ebp	\
	__asm pop edi	\
	__asm pop esi	\
	__asm pop ebx	\
	__asm ret


#endif

#pragma warning (disable:4100)
#pragma warning (disable:4799) // function has no EMMS instruction


extern int SpectrumSinusTable[256*2];

// -> CODE#0024 UPDATE#04
// -> DESC="wav export update"
//const float _f2ic = (float)(1 << 28);
//const float _i2fc = (float)(1.0 / (1 << 28));
//const float _f2ic = (float)0x7fffffff;
//const float _i2fc = (float)(1.0 / 0x7fffffff);
//const float _f2ic = (float)MIXING_CLIPMAX;
//const float _i2fc = (float)(1.0/MIXING_CLIPMAX);
// -! NEW_FEATURE#0024

static unsigned int QueryProcessorExtensions()
{
	static unsigned int fProcessorExtensions = 0;
	static bool bMMXChecked = false;

	if (!bMMXChecked)
	{
		_asm 
		{
			pushfd                      // Store original EFLAGS on stack
			pop     eax                 // Get original EFLAGS in EAX
			mov     ecx, eax            // Duplicate original EFLAGS in ECX for toggle check
			xor     eax, 0x00200000L    // Flip ID bit in EFLAGS
			push    eax                 // Save new EFLAGS value on stack
			popfd                       // Replace current EFLAGS value
			pushfd                      // Store new EFLAGS on stack
			pop     eax                 // Get new EFLAGS in EAX
			xor     eax, ecx            // Can we toggle ID bit?
			jz      Done                // Jump if no, Processor is older than a Pentium so CPU_ID is not supported
			mov		fProcessorExtensions, PROCSUPPORT_CPUID
			mov     eax, 1              // Set EAX to tell the CPUID instruction what to return
			push	ebx
			cpuid                       // Get family/model/stepping/features
			pop		ebx
			test    edx, 0x00800000L    // Check if mmx technology available
			jz      Done                // Jump if no
			// Tests have passed, this machine supports the Intel MultiMedia Instruction Set!
			or		fProcessorExtensions, PROCSUPPORT_MMX
			// Check for SSE: PROCSUPPORT_SSE
            test    edx, 0x02000000L    // check if SSE is present (bit 25)
            jz      nosse               // done if no
            // else set the correct bit in fProcessorExtensions
            or      fProcessorExtensions, (PROCSUPPORT_MMXEX|PROCSUPPORT_SSE)
			jmp     Done
		nosse:
			// Check for AMD 3DNow!
			mov eax, 0x80000000
			cpuid
			cmp eax, 0x80000000
			jbe Done
			mov eax, 0x80000001
			cpuid // CPU_ID
			test edx, 0x80000000
			jz Done
			or      fProcessorExtensions, PROCSUPPORT_3DNOW	// 3DNow! supported
			test edx, (1<<22) // Bit 22: AMD MMX extensions
			jz Done
			or      fProcessorExtensions, PROCSUPPORT_MMXEX	// MMX extensions supported
		}
Done:
		bMMXChecked = true;
    }
    return fProcessorExtensions;
}


DWORD CSoundFile::InitSysInfo()
//-----------------------------
{
	OSVERSIONINFO osvi;
	DWORD d = 0;

#ifdef _DEBUG
	// Must be aligned on 32 bytes for best performance
	if (sizeof(MODCHANNEL) & 0x1F)
	{
		CHAR s[64];
		wsprintf(s, "MODCHANNEL not aligned: sizeof(MODCHANNEL) = %d", sizeof(MODCHANNEL));
		::MessageBox(NULL, s, NULL, MB_OK|MB_ICONEXCLAMATION);
	}
	DWORD dwFastSinc = (DWORD)(LPVOID)gFastSinc;
	if (dwFastSinc & 7)
	{
		CHAR s[64];
		wsprintf(s, "gFastSinc is not aligned (%08X)!", dwFastSinc);
		::MessageBox(NULL, s, NULL, MB_OK|MB_ICONEXCLAMATION);
	}
#endif
	memset(&osvi, 0, sizeof(osvi));
	osvi.dwOSVersionInfoSize = sizeof(osvi);
	GetVersionEx(&osvi);
	DWORD dwProcSupport = QueryProcessorExtensions();
	switch(osvi.dwPlatformId)
	{
	// Don't use MMX for Windows 3.1
	case VER_PLATFORM_WIN32s:
		dwProcSupport &= PROCSUPPORT_CPUID;
		break;
	// XMM requires Windows 98
	case VER_PLATFORM_WIN32_WINDOWS:
		if ((osvi.dwMajorVersion < 4) || ((osvi.dwMajorVersion==4) && (!osvi.dwMinorVersion)))
		{
			dwProcSupport &= PROCSUPPORT_CPUID|PROCSUPPORT_MMX|PROCSUPPORT_3DNOW|PROCSUPPORT_MMXEX;
		}
		break;
	// XMM requires Windows 2000
	case VER_PLATFORM_WIN32_NT:
		if (osvi.dwMajorVersion < 5)
		{
			dwProcSupport &= PROCSUPPORT_CPUID|PROCSUPPORT_MMX|PROCSUPPORT_3DNOW|PROCSUPPORT_MMXEX;
		}
	}
	if (dwProcSupport & PROCSUPPORT_MMX) d |= (SYSMIX_ENABLEMMX|SYSMIX_FASTCPU);
	if (dwProcSupport & PROCSUPPORT_MMXEX) d |= SYSMIX_MMXEX;
	if (dwProcSupport & PROCSUPPORT_3DNOW) d |= SYSMIX_3DNOW;
	if (dwProcSupport & PROCSUPPORT_SSE) d |= SYSMIX_SSE;
	if (!(dwProcSupport & PROCSUPPORT_CPUID)) d |= SYSMIX_SLOWCPU;
	gdwSysInfo = d;
	return d;
}


#ifdef ENABLE_MMX



VOID MMX_EndMix()
{
	_asm {
	pxor mm0, mm0
	emms
	}
}


//////////////////////////////////////////////////////////////////////////
// Stereo MMX mixing - no interpolation

_declspec(naked) VOID __cdecl MMX_Mono8BitMix(MODCHANNEL *pChannel, int *pbuffer, int *pbufmax)
{
	_asm {
	MMX_ENTER
	mov eax, 0xFFFF
	movd mm6, eax
	mov ecx, MMX_PCHANNEL
	mov edi, MMX_PBUFFER
	mov edx, MMX_PBUFMAX
	mov esi, [ecx+CHNOFS_PCURRENTSAMPLE]
	add esi, [ecx+CHNOFS_NPOS]
	movzx ebx, word ptr [ecx+CHNOFS_NPOSLO]
	mov ebp, [ecx+CHNOFS_NINC]
	punpckldq mm6, mm6
	movq mm4, qword ptr [ecx+CHNOFS_NRIGHTVOL]	//; mm4 = [ leftvol | rightvol ]
	pand mm4, mm6
	mov eax, edx
	sub eax, edi
	push ecx
	test eax, 8
	jz mixloop
	mov eax, ebx
	sar eax,16
	movsx eax, byte ptr [esi+eax]
	add edi, 8
	shl eax, 8
	movd mm0, eax
	punpckldq mm0, mm0
	pmaddwd mm0, mm4
	paddd mm0, qword ptr [edi-8]
	add ebx, ebp
	cmp edi, edx
	movq qword ptr [edi-8], mm0
	jb mixloop
	jmp done
	align 16
mixloop:
	mov eax, ebx
	add ebx, ebp
	sar eax,16
	mov ecx, ebx
	movsx eax, byte ptr [esi+eax]
	sar ecx, 16
	add edi, 16
	movsx ecx, byte ptr [esi+ecx]
	shl eax, 8
	add ebx, ebp
	movd mm1, eax
	shl ecx, 8
	punpckldq mm1, mm1
	movd mm0, ecx
	pmaddwd mm1, mm4
	punpckldq mm0, mm0
	paddd mm1, qword ptr [edi-16]
	pmaddwd mm0, mm4
	paddd mm0, qword ptr [edi-8]
	cmp edi, edx
	movq qword ptr [edi-16], mm1
	movq qword ptr [edi-8], mm0
	jb mixloop
done:
	pop ecx
	mov word ptr [ecx+CHNOFS_NPOSLO], bx
	sar ebx, 16
	add dword ptr [ecx+CHNOFS_NPOS], ebx
	MMX_LEAVE
	}
}


_declspec(naked) VOID __cdecl MMX_Mono16BitMix(MODCHANNEL *pChannel, int *pbuffer, int *pbufmax)
{
	_asm {
	MMX_ENTER
	mov eax, 0xFFFF
	movd mm6, eax
	mov ecx, MMX_PCHANNEL
	mov edi, MMX_PBUFFER
	mov edx, MMX_PBUFMAX
	mov esi, [ecx+CHNOFS_PCURRENTSAMPLE]
	mov eax, [ecx+CHNOFS_NPOS]
	lea esi, [esi+eax*2]
	movzx ebx, word ptr [ecx+CHNOFS_NPOSLO]
	mov ebp, [ecx+CHNOFS_NINC]
	punpckldq mm6, mm6
	movq mm4, dword ptr [ecx+CHNOFS_NRIGHTVOL]	// mm4 = [ leftvol | rightvol ]
	pand mm4, mm6
	mov eax, edx
	sub eax, edi
	push ecx
	test eax, 8
	jz mixloop
	mov eax, ebx
	sar eax,16
	movsx eax, word ptr [esi+eax*2]
	add edi, 8
	movd mm0, eax
	punpckldq mm0, mm0
	pmaddwd mm0, mm4
	paddd mm0, qword ptr [edi-8]
	add ebx, ebp
	cmp edi, edx
	movq qword ptr [edi-8], mm0
	jb mixloop
	jmp done
	align 16
mixloop:
	mov eax, ebx
	add ebx, ebp
	sar eax, 16
	mov ecx, ebx
	movsx eax, word ptr [esi+eax*2]
	sar ecx, 16
	add edi, 16
	movsx ecx, word ptr [esi+ecx*2]
	movd mm1, eax
	add ebx, ebp
	punpckldq mm1, mm1
	movd mm0, ecx
	pmaddwd mm1, mm4
	punpckldq mm0, mm0
	pmaddwd mm0, mm4
	paddd mm1, qword ptr [edi-16]
	paddd mm0, qword ptr [edi-8]
	cmp edi, edx
	movq qword ptr [edi-16], mm1
	movq qword ptr [edi-8], mm0
	jb mixloop
done:
	pop ecx
	mov word ptr [ecx+CHNOFS_NPOSLO], bx
	sar ebx, 16
	add dword ptr [ecx+CHNOFS_NPOS], ebx
	MMX_LEAVE
	}
}


/////////////////////////////////////////////////////////////////////////////////////
// Linear interpolation


_declspec(naked) VOID __cdecl MMX_Mono8BitLinearMix(MODCHANNEL *pChannel, int *pbuffer, int *pbufmax)
{
	_asm {
	MMX_ENTER
	mov ecx, MMX_PCHANNEL
	mov edi, MMX_PBUFFER
	mov esi, [ecx+CHNOFS_PCURRENTSAMPLE]
	add esi, [ecx+CHNOFS_NPOS]
	movzx ebx, word ptr [ecx+CHNOFS_NPOSLO]
	mov ebp, [ecx+CHNOFS_NINC]
	mov eax, 0x0000FFFF
	movq mm4, qword ptr [ecx+CHNOFS_NRIGHTVOL]	// mm4 = [ leftvol | rightvol ]
	movd mm6, eax
	punpckldq mm6, mm6
	pand mm4, mm6
	mov ecx, MMX_PBUFMAX
mixloop:
	mov eax, ebx
	sar eax, 16
	movsx edx, byte ptr [esi+eax]
	movsx eax, byte ptr [esi+eax+1]
	add edi, 8
	movd mm1, eax
	movd mm0, edx
	movzx edx, bh
	psubd mm1, mm0
	movd mm2, edx
	pslld mm0, 8
	pmaddwd mm1, mm2	// mm1 = poslo * (s2-s1)
	paddd mm0, mm1
	punpckldq mm0, mm0
	pmaddwd mm0, mm4
	paddd mm0, qword ptr [edi-8]
	add ebx, ebp
	cmp edi, ecx
	movq qword ptr [edi-8], mm0
	jb mixloop
	mov ecx, MMX_PCHANNEL
	mov word ptr [ecx+CHNOFS_NPOSLO], bx
	sar ebx, 16
	add dword ptr [ecx+CHNOFS_NPOS], ebx
	MMX_LEAVE
	}
}


_declspec(naked) VOID __cdecl MMX_Mono16BitLinearMix(MODCHANNEL *pChannel, int *pbuffer, int *pbufmax)
{
	_asm {
	MMX_ENTER
	mov ecx, MMX_PCHANNEL
	mov edi, MMX_PBUFFER
	mov esi, [ecx+CHNOFS_PCURRENTSAMPLE]
	mov eax, [ecx+CHNOFS_NPOS]
	lea esi, [esi+eax*2]
	movzx ebx, word ptr [ecx+CHNOFS_NPOSLO]
	mov ebp, [ecx+CHNOFS_NINC]
	mov eax, 0x0000FFFF
	movq mm4, qword ptr [ecx+CHNOFS_NRIGHTVOL]	// mm4 = [ leftvol | rightvol ]
	movd mm6, eax
	punpckldq mm6, mm6
	pand mm4, mm6
	mov ecx, MMX_PBUFMAX
mixloop:
	mov eax, ebx
	sar eax, 16
	movsx edx, word ptr [esi+eax*2]
	movsx eax, word ptr [esi+eax*2+2]
	add edi, 8
	movd mm1, eax
	movzx eax, bh
	movd mm0, edx
	movd mm2, eax
	psubsw mm1, mm0
	pmaddwd mm1, mm2	// mm1 = poslo * (s2-s1)
	psrad mm1, 8
	paddd mm0, mm1
	punpckldq mm0, mm0
	pmaddwd mm0, mm4
	paddd mm0, qword ptr [edi-8]
	add ebx, ebp
	cmp edi, ecx
	movq qword ptr [edi-8], mm0
	jb mixloop
	mov ecx, MMX_PCHANNEL
	mov word ptr [ecx+CHNOFS_NPOSLO], bx
	sar ebx, 16
	add dword ptr [ecx+CHNOFS_NPOS], ebx
	MMX_LEAVE
	}
}


////////////////////////////////////////////////////////////////////////////////
// 4-taps polyphase FIR resampling filter

_declspec(naked) VOID __cdecl MMX_Mono8BitHQMix(MODCHANNEL *pChannel, int *pbuffer, int *pbufmax)
{
	_asm {
	MMX_ENTER
	mov eax, 0xFFFF
	movd mm6, eax
	mov ecx, MMX_PCHANNEL
	mov edi, MMX_PBUFFER
	mov edx, MMX_PBUFMAX
	mov esi, [ecx+CHNOFS_PCURRENTSAMPLE]
	add esi, [ecx+CHNOFS_NPOS]
	movzx ebx, word ptr [ecx+CHNOFS_NPOSLO]
	dec esi
	mov ebp, [ecx+CHNOFS_NINC]
	punpckldq mm6, mm6
	movq mm4, qword ptr [ecx+CHNOFS_NRIGHTVOL]	//; mm4 = [ leftvol | rightvol ]
	pand mm4, mm6
	paddsw mm4, mm4
mixloop:
	mov eax, ebx
	sar eax, 16
	movzx ecx, bh
	movd mm0, dword ptr [esi+eax]
	pxor mm7, mm7
	movq mm1, qword ptr [gFastSinc+ecx*8]	// mm1 = [ c3 | c2 | c1 | c0 ]
	punpcklbw mm7, mm0					// mm7 = [ s3 | s2 | s1 | s0 ]
	pmaddwd mm7, mm1					// mm7 = [c3*s3+c2*s2|c1*s1+c0*s0]
	add edi, 8
	movq mm0, mm7
	psrlq mm7, 32
	paddd mm0, mm7
	punpckldq mm0, mm0
	psrad mm0, 15
	packssdw mm0, mm0
	pmaddwd mm0, mm4
	paddd mm0, qword ptr [edi-8]
	add ebx, ebp
	cmp edi, edx
	movq qword ptr [edi-8], mm0
	jb mixloop
	mov ecx, MMX_PCHANNEL
	mov word ptr [ecx+CHNOFS_NPOSLO], bx
	sar ebx, 16
	add dword ptr [ecx+CHNOFS_NPOS], ebx
	MMX_LEAVE
	}
}


_declspec(naked) VOID __cdecl MMX_Mono16BitHQMix(MODCHANNEL *pChannel, int *pbuffer, int *pbufmax)
{
	_asm {
	MMX_ENTER
	mov eax, 0xFFFF
	movd mm6, eax
	mov ecx, MMX_PCHANNEL
	mov edi, MMX_PBUFFER
	mov edx, MMX_PBUFMAX
	mov esi, [ecx+CHNOFS_PCURRENTSAMPLE]
	mov eax, [ecx+CHNOFS_NPOS]
	lea esi, [esi+eax*2-2]
	movzx ebx, word ptr [ecx+CHNOFS_NPOSLO]
	mov ebp, [ecx+CHNOFS_NINC]
	punpckldq mm6, mm6
	movq mm4, dword ptr [ecx+CHNOFS_NRIGHTVOL]	// mm4 = [ leftvol | rightvol ]
	pand mm4, mm6
mixloop:
	mov eax, ebx
	sar eax, 16
	movzx ecx, bh
	movq mm7, qword ptr [esi+eax*2]		// mm7 = [ s3 | s2 | s1 | s0 ]
	movq mm1, qword ptr [gFastSinc+ecx*8] // mm1 = [ c3 | c2 | c1 | c0 ]
	add edi, 8
	pmaddwd mm7, mm1					// mm7 = [c3*s3+c2*s2|c1*s1+c0*s0]
	movq mm0, mm7
	psrlq mm7, 32
	paddd mm0, mm7
	punpckldq mm0, mm0
	psrad mm0, 14
	packssdw mm0, mm0
	pmaddwd mm0, mm4
	paddd mm0, qword ptr [edi-8]
	add ebx, ebp
	cmp edi, edx
	movq qword ptr [edi-8], mm0
	jb mixloop
	mov ecx, MMX_PCHANNEL
	mov word ptr [ecx+CHNOFS_NPOSLO], bx
	sar ebx, 16
	add dword ptr [ecx+CHNOFS_NPOS], ebx
	MMX_LEAVE
	}
}

////////////////////////////////////////////////////////////////////////////////
// 8-taps polyphase FIR resampling filter

_declspec(naked) VOID __cdecl MMX_Mono8BitKaiserMix(MODCHANNEL *pChannel, int *pbuffer, int *pbufmax)
{
	_asm {
	MMX_ENTER
	mov eax, 0xFFFF
	movd mm6, eax
	mov ecx, MMX_PCHANNEL
	mov edi, MMX_PBUFFER
	mov edx, MMX_PBUFMAX
	mov esi, [ecx+CHNOFS_PCURRENTSAMPLE]
	add esi, [ecx+CHNOFS_NPOS]
	movzx ebx, word ptr [ecx+CHNOFS_NPOSLO]
	sub esi, 3
	mov ebp, [ecx+CHNOFS_NINC]
	punpckldq mm6, mm6
	movq mm4, qword ptr [ecx+CHNOFS_NRIGHTVOL]	//; mm4 = [ leftvol | rightvol ]
	pand mm4, mm6
	paddsw mm4, mm4
	cmp ebp, 0x18000
	jg mixloop2x
	cmp ebp, -0x18000
	jl mixloop2x
	cmp ebp, 0x13000
	jg mixloop3x
	cmp ebp, -0x13000
	jl mixloop3x
mixloop:
	mov eax, ebx
	mov ecx, ebx
	sar eax, 16
	and ecx, 0xfff0
	movq mm0, qword ptr [esi+eax]		// mm0 = [s7|s6|s5|s4|s3|s2|s1|s0]
	pxor mm6, mm6
	movq mm1, qword ptr [gKaiserSinc+ecx]	// mm1 = [ c3 | c2 | c1 | c0 ]
	movq mm2, qword ptr [gKaiserSinc+ecx+8]	// mm2 = [ c7 | c6 | c5 | c4 ]
	punpcklbw mm6, mm0					// mm6 = [ s3 | s2 | s1 | s0 ]
	pxor mm7, mm7
	pmaddwd mm6, mm1
	punpckhbw mm7, mm0					// mm7 = [ s7 | s6 | s5 | s4 ]
	pmaddwd mm7, mm2
	add edi, 8
	add ebx, ebp
	paddd mm6, mm7
	movq mm0, mm6
	psrlq mm6, 32
	paddd mm0, mm6
	psrad mm0, 15
	punpckldq mm0, mm0
	packssdw mm0, mm0
	pmaddwd mm0, mm4
	paddd mm0, qword ptr [edi-8]
	cmp edi, edx
	movq qword ptr [edi-8], mm0
	jb mixloop
	jmp done
mixloop2x:
	mov eax, ebx
	mov ecx, ebx
	sar eax, 16
	and ecx, 0xfff0
	movq mm0, qword ptr [esi+eax]		// mm0 = [s7|s6|s5|s4|s3|s2|s1|s0]
	pxor mm6, mm6
	movq mm1, qword ptr [gDownsample2x+ecx]	// mm1 = [ c3 | c2 | c1 | c0 ]
	movq mm2, qword ptr [gDownsample2x+ecx+8]	// mm2 = [ c7 | c6 | c5 | c4 ]
	punpcklbw mm6, mm0					// mm6 = [ s3 | s2 | s1 | s0 ]
	pxor mm7, mm7
	pmaddwd mm6, mm1
	punpckhbw mm7, mm0					// mm7 = [ s7 | s6 | s5 | s4 ]
	pmaddwd mm7, mm2
	add edi, 8
	add ebx, ebp
	paddd mm6, mm7
	movq mm0, mm6
	psrlq mm6, 32
	paddd mm0, mm6
	psrad mm0, 15
	punpckldq mm0, mm0
	packssdw mm0, mm0
	pmaddwd mm0, mm4
	paddd mm0, qword ptr [edi-8]
	cmp edi, edx
	movq qword ptr [edi-8], mm0
	jb mixloop2x
	jmp done
mixloop3x:
	mov eax, ebx
	mov ecx, ebx
	sar eax, 16
	and ecx, 0xfff0
	movq mm0, qword ptr [esi+eax]		// mm0 = [s7|s6|s5|s4|s3|s2|s1|s0]
	pxor mm6, mm6
	movq mm1, qword ptr [gDownsample13x+ecx]	// mm1 = [ c3 | c2 | c1 | c0 ]
	movq mm2, qword ptr [gDownsample13x+ecx+8]	// mm2 = [ c7 | c6 | c5 | c4 ]
	punpcklbw mm6, mm0					// mm6 = [ s3 | s2 | s1 | s0 ]
	pxor mm7, mm7
	pmaddwd mm6, mm1
	punpckhbw mm7, mm0					// mm7 = [ s7 | s6 | s5 | s4 ]
	pmaddwd mm7, mm2
	add edi, 8
	add ebx, ebp
	paddd mm6, mm7
	movq mm0, mm6
	psrlq mm6, 32
	paddd mm0, mm6
	psrad mm0, 15
	punpckldq mm0, mm0
	packssdw mm0, mm0
	pmaddwd mm0, mm4
	paddd mm0, qword ptr [edi-8]
	cmp edi, edx
	movq qword ptr [edi-8], mm0
	jb mixloop3x
done:
	mov ecx, MMX_PCHANNEL
	mov word ptr [ecx+CHNOFS_NPOSLO], bx
	sar ebx, 16
	add dword ptr [ecx+CHNOFS_NPOS], ebx
	MMX_LEAVE
	}
}


_declspec(naked) VOID __cdecl MMX_Mono8BitKaiserRampMix(MODCHANNEL *pChannel, int *pbuffer, int *pbufmax)
{
	_asm {
	MMX_ENTER
	mov ecx, MMX_PCHANNEL
	mov edi, MMX_PBUFFER
	mov edx, MMX_PBUFMAX
	mov esi, [ecx+CHNOFS_PCURRENTSAMPLE]
	add esi, [ecx+CHNOFS_NPOS]
	movzx ebx, word ptr [ecx+CHNOFS_NPOSLO]
	sub esi, 3
	mov ebp, [ecx+CHNOFS_NINC]
	movq mm4, qword ptr [ecx+MODCHANNEL.nRampRightVol]	// mm4 = [ leftvol  | rightvol  ]
	movq mm3, qword ptr [ecx+MODCHANNEL.nRightRamp]		// mm3 = [ leftramp | rightramp ]
	cmp ebp, 0x18000
	jg mixloop2x
	cmp ebp, -0x18000
	jl mixloop2x
	cmp ebp, 0x13000
	jg mixloop3x
	cmp ebp, -0x13000
	jl mixloop3x
mixloop:
	mov eax, ebx
	mov ecx, ebx
	sar eax, 16
	and ecx, 0xfff0
	movq mm0, qword ptr [esi+eax]		// mm0 = [s7|s6|s5|s4|s3|s2|s1|s0]
	pxor mm6, mm6
	movq mm1, qword ptr [gKaiserSinc+ecx]	// mm1 = [ c3 | c2 | c1 | c0 ]
	movq mm2, qword ptr [gKaiserSinc+ecx+8]	// mm2 = [ c7 | c6 | c5 | c4 ]
	punpcklbw mm6, mm0						// mm6 = [ s3 | s2 | s1 | s0 ]
	pxor mm7, mm7
	pmaddwd mm6, mm1
	punpckhbw mm7, mm0						// mm7 = [ s7 | s6 | s5 | s4 ]
	pmaddwd mm7, mm2
	paddd mm4, mm3							// nRampVol += nRamp
	add edi, 8
	add ebx, ebp
	paddd mm6, mm7
	movq mm7, mm4
	movq mm0, mm6
	psrlq mm6, 32
	psrad mm7, VOLUMERAMPPRECISION-1		// nRampVol >> VOLUMERAMPPRECISION-1
	paddd mm0, mm6
	pxor mm6, mm6
	psrad mm0, 15
	packssdw mm7, mm7
	punpckldq mm0, mm0
	punpcklwd mm7, mm6
	packssdw mm0, mm0
	pmaddwd mm0, mm7
	paddd mm0, qword ptr [edi-8]
	cmp edi, edx
	movq qword ptr [edi-8], mm0
	jb mixloop
	jmp done
mixloop2x:
	mov eax, ebx
	mov ecx, ebx
	sar eax, 16
	and ecx, 0xfff0
	movq mm0, qword ptr [esi+eax]		// mm0 = [s7|s6|s5|s4|s3|s2|s1|s0]
	pxor mm6, mm6
	movq mm1, qword ptr [gDownsample2x+ecx]	// mm1 = [ c3 | c2 | c1 | c0 ]
	movq mm2, qword ptr [gDownsample2x+ecx+8]	// mm2 = [ c7 | c6 | c5 | c4 ]
	punpcklbw mm6, mm0						// mm6 = [ s3 | s2 | s1 | s0 ]
	pxor mm7, mm7
	pmaddwd mm6, mm1
	punpckhbw mm7, mm0						// mm7 = [ s7 | s6 | s5 | s4 ]
	pmaddwd mm7, mm2
	paddd mm4, mm3							// nRampVol += nRamp
	add edi, 8
	add ebx, ebp
	paddd mm6, mm7
	movq mm7, mm4
	movq mm0, mm6
	psrlq mm6, 32
	psrad mm7, VOLUMERAMPPRECISION-1		// nRampVol >> VOLUMERAMPPRECISION-1
	paddd mm0, mm6
	pxor mm6, mm6
	psrad mm0, 15
	packssdw mm7, mm7
	punpckldq mm0, mm0
	punpcklwd mm7, mm6
	packssdw mm0, mm0
	pmaddwd mm0, mm7
	paddd mm0, qword ptr [edi-8]
	cmp edi, edx
	movq qword ptr [edi-8], mm0
	jb mixloop2x
	jmp done
mixloop3x:
	mov eax, ebx
	mov ecx, ebx
	sar eax, 16
	and ecx, 0xfff0
	movq mm0, qword ptr [esi+eax]		// mm0 = [s7|s6|s5|s4|s3|s2|s1|s0]
	pxor mm6, mm6
	movq mm1, qword ptr [gDownsample13x+ecx]	// mm1 = [ c3 | c2 | c1 | c0 ]
	movq mm2, qword ptr [gDownsample13x+ecx+8]	// mm2 = [ c7 | c6 | c5 | c4 ]
	punpcklbw mm6, mm0						// mm6 = [ s3 | s2 | s1 | s0 ]
	pxor mm7, mm7
	pmaddwd mm6, mm1
	punpckhbw mm7, mm0						// mm7 = [ s7 | s6 | s5 | s4 ]
	pmaddwd mm7, mm2
	paddd mm4, mm3							// nRampVol += nRamp
	add edi, 8
	add ebx, ebp
	paddd mm6, mm7
	movq mm7, mm4
	movq mm0, mm6
	psrlq mm6, 32
	psrad mm7, VOLUMERAMPPRECISION-1		// nRampVol >> VOLUMERAMPPRECISION-1
	paddd mm0, mm6
	pxor mm6, mm6
	psrad mm0, 15
	packssdw mm7, mm7
	punpckldq mm0, mm0
	punpcklwd mm7, mm6
	packssdw mm0, mm0
	pmaddwd mm0, mm7
	paddd mm0, qword ptr [edi-8]
	cmp edi, edx
	movq qword ptr [edi-8], mm0
	jb mixloop3x
done:
	mov ecx, MMX_PCHANNEL
	mov word ptr [ecx+CHNOFS_NPOSLO], bx
	sar ebx, 16
	add dword ptr [ecx+CHNOFS_NPOS], ebx
	movq qword ptr [ecx+MODCHANNEL.nRampRightVol], mm4
	psrad mm4, VOLUMERAMPPRECISION
	movq qword ptr [ecx+MODCHANNEL.nRightVol], mm4
	MMX_LEAVE
	}
}


_declspec(naked) VOID __cdecl MMX_Mono16BitKaiserMix(MODCHANNEL *pChannel, int *pbuffer, int *pbufmax)
{
	_asm {
	MMX_ENTER
	mov eax, 0xFFFF
	movd mm6, eax
	mov ecx, MMX_PCHANNEL
	mov edi, MMX_PBUFFER
	mov edx, MMX_PBUFMAX
	mov esi, [ecx+CHNOFS_PCURRENTSAMPLE]
	mov eax, [ecx+CHNOFS_NPOS]
	lea esi, [esi+eax*2-6]
	movzx ebx, word ptr [ecx+CHNOFS_NPOSLO]
	mov ebp, [ecx+CHNOFS_NINC]
	punpckldq mm6, mm6
	movq mm4, dword ptr [ecx+CHNOFS_NRIGHTVOL]	// mm4 = [ leftvol | rightvol ]
	pand mm4, mm6
	cmp ebp, 0x18000
	jg mixloop2x
	cmp ebp, -0x18000
	jl mixloop2x
	cmp ebp, 0x13000
	jg mixloop3x
	cmp ebp, -0x13000
	jl mixloop3x
mixloop:
	mov eax, ebx
	mov ecx, ebx
	sar eax, 16
	and ecx, 0xfff0
	movq mm6, qword ptr [esi+eax*2]			// mm6 = [ s3 | s2 | s1 | s0 ]
	movq mm1, qword ptr [gKaiserSinc+ecx]	// mm1 = [ c3 | c2 | c1 | c0 ]
	movq mm7, qword ptr [esi+eax*2+8]		// mm7 = [ s7 | s6 | s5 | s4 ]
	movq mm2, qword ptr [gKaiserSinc+ecx+8]	// mm2 = [ c7 | c6 | c5 | c4 ]
	pmaddwd mm6, mm1
	add edi, 8
	pmaddwd mm7, mm2
	add ebx, ebp
	paddd mm6, mm7
	movq mm0, mm6
	psrlq mm6, 32
	paddd mm0, mm6
	psrad mm0, 14
	punpckldq mm0, mm0
	packssdw mm0, mm0
	pmaddwd mm0, mm4
	paddd mm0, qword ptr [edi-8]
	cmp edi, edx
	movq qword ptr [edi-8], mm0
	jb mixloop
	jmp done
mixloop2x:
	mov eax, ebx
	mov ecx, ebx
	sar eax, 16
	and ecx, 0xfff0
	movq mm6, qword ptr [esi+eax*2]			// mm6 = [ s3 | s2 | s1 | s0 ]
	movq mm1, qword ptr [gDownsample2x+ecx]	// mm1 = [ c3 | c2 | c1 | c0 ]
	movq mm7, qword ptr [esi+eax*2+8]		// mm7 = [ s7 | s6 | s5 | s4 ]
	movq mm2, qword ptr [gDownsample2x+ecx+8]	// mm2 = [ c7 | c6 | c5 | c4 ]
	pmaddwd mm6, mm1
	add edi, 8
	pmaddwd mm7, mm2
	add ebx, ebp
	paddd mm6, mm7
	movq mm0, mm6
	psrlq mm6, 32
	paddd mm0, mm6
	psrad mm0, 14
	punpckldq mm0, mm0
	packssdw mm0, mm0
	pmaddwd mm0, mm4
	paddd mm0, qword ptr [edi-8]
	cmp edi, edx
	movq qword ptr [edi-8], mm0
	jb mixloop2x
	jmp done
mixloop3x:
	mov eax, ebx
	mov ecx, ebx
	sar eax, 16
	and ecx, 0xfff0
	movq mm6, qword ptr [esi+eax*2]			// mm6 = [ s3 | s2 | s1 | s0 ]
	movq mm1, qword ptr [gDownsample13x+ecx]	// mm1 = [ c3 | c2 | c1 | c0 ]
	movq mm7, qword ptr [esi+eax*2+8]		// mm7 = [ s7 | s6 | s5 | s4 ]
	movq mm2, qword ptr [gDownsample13x+ecx+8]	// mm2 = [ c7 | c6 | c5 | c4 ]
	pmaddwd mm6, mm1
	add edi, 8
	pmaddwd mm7, mm2
	add ebx, ebp
	paddd mm6, mm7
	movq mm0, mm6
	psrlq mm6, 32
	paddd mm0, mm6
	psrad mm0, 14
	punpckldq mm0, mm0
	packssdw mm0, mm0
	pmaddwd mm0, mm4
	paddd mm0, qword ptr [edi-8]
	cmp edi, edx
	movq qword ptr [edi-8], mm0
	jb mixloop3x
done:
	mov ecx, MMX_PCHANNEL
	mov word ptr [ecx+CHNOFS_NPOSLO], bx
	sar ebx, 16
	add dword ptr [ecx+CHNOFS_NPOS], ebx
	MMX_LEAVE
	}
}


_declspec(naked) VOID __cdecl MMX_Mono16BitKaiserRampMix(MODCHANNEL *pChannel, int *pbuffer, int *pbufmax)
{
	_asm {
	MMX_ENTER
	mov ecx, MMX_PCHANNEL
	mov edi, MMX_PBUFFER
	mov edx, MMX_PBUFMAX
	mov esi, [ecx+CHNOFS_PCURRENTSAMPLE]
	mov eax, [ecx+CHNOFS_NPOS]
	lea esi, [esi+eax*2-6]
	movzx ebx, word ptr [ecx+CHNOFS_NPOSLO]
	mov ebp, [ecx+CHNOFS_NINC]
	movq mm4, qword ptr [ecx+MODCHANNEL.nRampRightVol]	// mm4 = [ leftvol  | rightvol  ]
	movq mm3, qword ptr [ecx+MODCHANNEL.nRightRamp]		// mm3 = [ leftramp | rightramp ]
	cmp ebp, 0x18000
	jg mixloop2x
	cmp ebp, -0x18000
	jl mixloop2x
	cmp ebp, 0x13000
	jg mixloop3x
	cmp ebp, -0x13000
	jl mixloop3x
mixloop:
	mov eax, ebx
	mov ecx, ebx
	sar eax, 16
	and ecx, 0xfff0
	movq mm6, qword ptr [esi+eax*2]			// mm6 = [ s3 | s2 | s1 | s0 ]
	movq mm1, qword ptr [gKaiserSinc+ecx]	// mm1 = [ c3 | c2 | c1 | c0 ]
	movq mm7, qword ptr [esi+eax*2+8]		// mm7 = [ s7 | s6 | s5 | s4 ]
	movq mm2, qword ptr [gKaiserSinc+ecx+8]	// mm2 = [ c7 | c6 | c5 | c4 ]
	pmaddwd mm6, mm1
	add edi, 8
	pmaddwd mm7, mm2
	paddd mm4, mm3
	add ebx, ebp
	paddd mm6, mm7
	movq mm7, mm4
	movq mm0, mm6
	psrlq mm6, 32
	psrad mm7, VOLUMERAMPPRECISION		// nRampVol >> VOLUMERAMPPRECISION
	paddd mm0, mm6
	pxor mm6, mm6
	psrad mm0, 14
	packssdw mm7, mm7
	punpckldq mm0, mm0
	punpcklwd mm7, mm6
	packssdw mm0, mm0
	pmaddwd mm0, mm7
	paddd mm0, qword ptr [edi-8]
	cmp edi, edx
	movq qword ptr [edi-8], mm0
	jb mixloop
	jmp done
mixloop2x:
	mov eax, ebx
	mov ecx, ebx
	sar eax, 16
	and ecx, 0xfff0
	movq mm6, qword ptr [esi+eax*2]			// mm6 = [ s3 | s2 | s1 | s0 ]
	movq mm1, qword ptr [gDownsample2x+ecx]	// mm1 = [ c3 | c2 | c1 | c0 ]
	movq mm7, qword ptr [esi+eax*2+8]		// mm7 = [ s7 | s6 | s5 | s4 ]
	movq mm2, qword ptr [gDownsample2x+ecx+8]	// mm2 = [ c7 | c6 | c5 | c4 ]
	pmaddwd mm6, mm1
	add edi, 8
	pmaddwd mm7, mm2
	paddd mm4, mm3
	add ebx, ebp
	paddd mm6, mm7
	movq mm7, mm4
	movq mm0, mm6
	psrlq mm6, 32
	psrad mm7, VOLUMERAMPPRECISION		// nRampVol >> VOLUMERAMPPRECISION
	paddd mm0, mm6
	pxor mm6, mm6
	psrad mm0, 14
	packssdw mm7, mm7
	punpckldq mm0, mm0
	punpcklwd mm7, mm6
	packssdw mm0, mm0
	pmaddwd mm0, mm7
	paddd mm0, qword ptr [edi-8]
	cmp edi, edx
	movq qword ptr [edi-8], mm0
	jb mixloop2x
	jmp done
mixloop3x:
	mov eax, ebx
	mov ecx, ebx
	sar eax, 16
	and ecx, 0xfff0
	movq mm6, qword ptr [esi+eax*2]			// mm6 = [ s3 | s2 | s1 | s0 ]
	movq mm1, qword ptr [gDownsample13x+ecx]	// mm1 = [ c3 | c2 | c1 | c0 ]
	movq mm7, qword ptr [esi+eax*2+8]		// mm7 = [ s7 | s6 | s5 | s4 ]
	movq mm2, qword ptr [gDownsample13x+ecx+8]	// mm2 = [ c7 | c6 | c5 | c4 ]
	pmaddwd mm6, mm1
	add edi, 8
	pmaddwd mm7, mm2
	paddd mm4, mm3
	add ebx, ebp
	paddd mm6, mm7
	movq mm7, mm4
	movq mm0, mm6
	psrlq mm6, 32
	psrad mm7, VOLUMERAMPPRECISION		// nRampVol >> VOLUMERAMPPRECISION
	paddd mm0, mm6
	pxor mm6, mm6
	psrad mm0, 14
	packssdw mm7, mm7
	punpckldq mm0, mm0
	punpcklwd mm7, mm6
	packssdw mm0, mm0
	pmaddwd mm0, mm7
	paddd mm0, qword ptr [edi-8]
	cmp edi, edx
	movq qword ptr [edi-8], mm0
	jb mixloop3x
done:
	mov ecx, MMX_PCHANNEL
	mov word ptr [ecx+CHNOFS_NPOSLO], bx
	sar ebx, 16
	add dword ptr [ecx+CHNOFS_NPOS], ebx
	movq qword ptr [ecx+MODCHANNEL.nRampRightVol], mm4
	psrad mm4, VOLUMERAMPPRECISION
	movq qword ptr [ecx+MODCHANNEL.nRightVol], mm4
	MMX_LEAVE
	}
}


////////////////////////////////////////////////////////////////////////////////
// Filtered + linear interpolation + ramping

_declspec(naked) VOID __cdecl MMX_FilterMono8BitLinearRampMix(MODCHANNEL *pChannel, int *pbuffer, int *pbufmax)
{
	_asm {
	MMX_ENTER
	mov ecx, MMX_PCHANNEL
	mov edi, MMX_PBUFFER
	mov esi, [ecx+CHNOFS_PCURRENTSAMPLE]
	mov eax, [ecx+CHNOFS_NPOS]
	movzx ebx, word ptr [ecx+CHNOFS_NPOSLO]
	mov ebp, [ecx+CHNOFS_NINC]
	mov edx, [ecx+MODCHANNEL.nRampLength]
	add esi, eax
	or edx, edx
	movq mm4, qword ptr [ecx+MODCHANNEL.nRampRightVol]	// mm4 = [ leftvol | rightvol ]
	movq mm3, qword ptr [ecx+MODCHANNEL.nRightRamp]		// mm3 = [ leftramp | rightramp ]
	jnz noramp
	movq mm4, qword ptr [ecx+MODCHANNEL.nRightVol]
	pxor mm3, mm3
	pslld mm4, VOLUMERAMPPRECISION
noramp:
	movq mm6, qword ptr [ecx+MODCHANNEL.nFilter_Y1]		// mm6 = [ y2 | y1 ]
	movd mm0, dword ptr [ecx+MODCHANNEL.nFilter_B0]
	movd mm1, dword ptr [ecx+MODCHANNEL.nFilter_B1]
	punpckldq mm0, mm1									// mm0 = [ b1 | b0 ]
	movd mm7, dword ptr [ecx+MODCHANNEL.nFilter_HP]
	movd mm2, dword ptr [ecx+MODCHANNEL.nFilter_A0]
	punpckldq mm7, mm2									// mm7 = [ a0 | hp ]
	packssdw mm7, mm0									// mm7 = [ b1 | b0 | a0 | hp ]
	mov ecx, MMX_PBUFMAX
mixloop:
	mov eax, ebx
	sar eax, 16
	add edi, 8
	movsx edx, byte ptr [esi+eax]
	movsx eax, byte ptr [esi+eax]
	movd mm0, edx
	sub eax, edx
	movzx edx, bh
	imul edx, eax
	pslld mm0, 8
	movd mm1, edx
	movq mm5, mm7
	paddd mm0, mm1			// mm0 = s0+x*(s1-s0)
	pxor mm1, mm1
	psrad mm0, 1
	punpcklwd mm5, mm5				// mm5 = [ a0 | a0 | hp | hp ]
	packssdw mm1, mm6				// mm1 = [ y2 | y1 |  0 |  0 ]
	pand mm5, mm0					// mm5 = [    0    |  vol&hp ]
	pslld mm0, 16					// mm0 = [  0 |  0 | vol|  0 ]
	por mm0, mm1					// mm0 = [ y2 | y1 | vol|  0 ]
	pmaddwd mm0, mm7				// mm0 = [ y1*b0+y2*b1 | vol*a0 ]
	mov eax, 4096
	paddd mm4, mm3
	movd mm1, eax
	paddd mm1, mm0
	punpckhdq mm0, mm0
	paddd mm0, mm1
	psrad mm0, 13
	movq mm1, mm0
	punpckldq mm0, mm0
	psubd mm1, mm5
	movq mm5, qword ptr [edi-8]
	movq mm2, mm4
	psrad mm2, VOLUMERAMPPRECISION		// nRampVol >> VOLUMERAMPPRECISION
	packssdw mm2, mm2
	packssdw mm0, mm0
	punpcklwd mm2, mm2
	pmaddwd mm0, mm2
	punpckldq mm1, mm6
	movq mm6, mm1
	add ebx, ebp
	paddd mm0, mm5
	cmp edi, ecx
	movq qword ptr [edi-8], mm0
	jb mixloop
	mov ecx, MMX_PCHANNEL
	mov word ptr [ecx+CHNOFS_NPOSLO], bx
	sar ebx, 16
	add dword ptr [ecx+CHNOFS_NPOS], ebx
	movq qword ptr [ecx+MODCHANNEL.nRampRightVol], mm4
	psrad mm4, VOLUMERAMPPRECISION
	movq qword ptr [ecx+MODCHANNEL.nRightVol], mm4
	movq qword ptr [ecx+MODCHANNEL.nFilter_Y1], mm6
	MMX_LEAVE
	}
}


_declspec(naked) VOID __cdecl MMX_FilterMono16BitLinearRampMix(MODCHANNEL *pChannel, int *pbuffer, int *pbufmax)
{
	_asm {
	MMX_ENTER
	mov ecx, MMX_PCHANNEL
	mov edi, MMX_PBUFFER
	mov esi, [ecx+CHNOFS_PCURRENTSAMPLE]
	mov eax, [ecx+CHNOFS_NPOS]
	movzx ebx, word ptr [ecx+CHNOFS_NPOSLO]
	mov ebp, [ecx+CHNOFS_NINC]
	mov edx, [ecx+MODCHANNEL.nRampLength]
	lea esi, [esi+eax*2]
	or edx, edx
	movq mm4, qword ptr [ecx+MODCHANNEL.nRampRightVol]	// mm4 = [ leftvol | rightvol ]
	movq mm3, qword ptr [ecx+MODCHANNEL.nRightRamp]		// mm3 = [ leftramp | rightramp ]
	jnz noramp
	movq mm4, qword ptr [ecx+MODCHANNEL.nRightVol]
	pxor mm3, mm3
	pslld mm4, VOLUMERAMPPRECISION
noramp:
	movq mm6, qword ptr [ecx+MODCHANNEL.nFilter_Y1]		// mm6 = [ y2 | y1 ]
	movd mm0, dword ptr [ecx+MODCHANNEL.nFilter_B0]
	movd mm1, dword ptr [ecx+MODCHANNEL.nFilter_B1]
	punpckldq mm0, mm1									// mm0 = [ b1 | b0 ]
	movd mm7, dword ptr [ecx+MODCHANNEL.nFilter_HP]
	movd mm2, dword ptr [ecx+MODCHANNEL.nFilter_A0]
	punpckldq mm7, mm2									// mm7 = [ a0 | hp ]
	packssdw mm7, mm0									// mm7 = [ b1 | b0 | a0 | hp ]
	mov ecx, MMX_PBUFMAX
mixloop:
	mov eax, ebx
	sar eax, 16
	add edi, 8
	movd mm0, dword ptr [esi+eax*2]	// mm0 = [  0 |  0 | s1 | s0 ]
	movzx edx, bh					// edx = x
	mov eax, 0x100
	sub eax, edx					// eax = 1-x
	shl edx, 16
	or eax, edx						// eax = [  x  | 1-x ]
	movd mm1, eax					// mm1 = [  0  |  0 |  x  | 1-x ]
	pmaddwd mm0, mm1				// mm0 = [     0    | x*s1+(1-x)*s0 ]
	movq mm5, mm7
	pxor mm1, mm1
	psrad mm0, 9
	punpcklwd mm5, mm5				// mm5 = [ a0 | a0 | hp | hp ]
	packssdw mm1, mm6				// mm1 = [ y2 | y1 |  0 |  0 ]
	pand mm5, mm0					// mm5 = [    0    |  vol&hp ]
	pslld mm0, 16					// mm0 = [  0 |  0 | vol|  0 ]
	por mm0, mm1					// mm0 = [ y2 | y1 | vol|  0 ]
	pmaddwd mm0, mm7				// mm0 = [ y1*b0+y2*b1 | vol*a0 ]
	mov eax, 4096
	paddd mm4, mm3
	movd mm1, eax
	paddd mm1, mm0
	punpckhdq mm0, mm0
	paddd mm0, mm1
	psrad mm0, 13
	movq mm1, mm0
	punpckldq mm0, mm0
	psubd mm1, mm5
	movq mm5, qword ptr [edi-8]
	movq mm2, mm4
	psrad mm2, VOLUMERAMPPRECISION		// nRampVol >> VOLUMERAMPPRECISION
	packssdw mm2, mm2
	packssdw mm0, mm0
	punpcklwd mm2, mm2
	pmaddwd mm0, mm2
	punpckldq mm1, mm6
	movq mm6, mm1
	add ebx, ebp
	paddd mm0, mm5
	cmp edi, ecx
	movq qword ptr [edi-8], mm0
	jb mixloop
	mov ecx, MMX_PCHANNEL
	mov word ptr [ecx+CHNOFS_NPOSLO], bx
	sar ebx, 16
	add dword ptr [ecx+CHNOFS_NPOS], ebx
	movq qword ptr [ecx+MODCHANNEL.nRampRightVol], mm4
	psrad mm4, VOLUMERAMPPRECISION
	movq qword ptr [ecx+MODCHANNEL.nRightVol], mm4
	movq qword ptr [ecx+MODCHANNEL.nFilter_Y1], mm6
	MMX_LEAVE
	}
}



//////////////////////////////////////////////////////////////////////////////


// MMX_Spectrum(signed char *pBuffer, UINT nSamples, UINT nInc, UINT nSmpSize, LPLONG lpSinCos)
__declspec(naked) void __cdecl MMX_Spectrum(signed char *, UINT, UINT, UINT, LPLONG)
//----------------------------------------------------------------------------------
{
	__asm {
	MMX_ENTER
	mov eax, 0x40*4
	mov edi, MMX_PARAM1		// edi = s(t)
	mov ecx, MMX_PARAM2		// ecx = nSamples
	movd mm4, eax			// mm4 = wt
	mov eax, MMX_PARAM3
	shl eax, 2
	movd mm5, eax			// mm5 = w
	xor eax, eax
	mov ebx, MMX_PARAM4		// ebx = nInc
	movd mm2, eax			// mm2 = E[s(t).cos(wt)|s(t).sin(wt)]
	movd ebp, mm4
SpectrumLoop:
	movsx edx, byte ptr [edi]	// edx = s(t)
	and ebp, 0x1FF*4
	and edx, 0xFFFF
	movd mm0, dword ptr SpectrumSinusTable[ebp]
	add ebp, 0x80*4
	add edi, ebx
	and ebp, 0x1FF*4
	movd mm6, edx
	movd mm1, dword ptr SpectrumSinusTable[ebp]
	punpckldq mm6, mm6		// mm6 = [   s(t)  |   s(t)  ]
	punpckldq mm0, mm1		// mm0 = [ cos(wt) | sin(wt) ]
	pmaddwd mm0, mm6		// mm0 = [s(t).cos(wt)|s(t).sin(wt)]
	paddd mm4, mm5
	paddd mm2, mm0
	dec ecx
	movd ebp, mm4
	jnz SpectrumLoop
	mov eax, MMX_PARAM5
	movq qword ptr [eax], mm2
	emms
	MMX_LEAVE
	}
}


#endif  // MMX code

//////////////////////////////////////////////////////////////////////////////////
//
// Misc. mix functions
//



////////////////////////////////////////////////////////////////////////////////////
// 3DNow! optimizations

#ifdef ENABLE_AMDNOW

// Convert integer mix to floating-point
void AMD_StereoMixToFloat(const int *pSrc, float *pOut1, float *pOut2, UINT nCount, const float _i2fc)
//----------------------------------------------------------------------------------------------------
{
	_asm {
	movd mm0, _i2fc
	mov edx, pSrc
	mov edi, pOut1
	mov ebx, pOut2
	mov ecx, nCount
	punpckldq mm0, mm0
	inc ecx
	shr ecx, 1
mainloop:
	movq mm1, qword ptr [edx]
	movq mm2, qword ptr [edx+8]
	add edi, 8
	pi2fd mm1, mm1
	pi2fd mm2, mm2
	add ebx, 8
	pfmul mm1, mm0
	pfmul mm2, mm0
	add edx, 16
	movq mm3, mm1
	punpckldq mm3, mm2
	punpckhdq mm1, mm2
	dec ecx
	movq qword ptr [edi-8], mm3
	movq qword ptr [ebx-8], mm1
	jnz mainloop
	emms
	}
}

void AMD_FloatToStereoMix(const float *pIn1, const float *pIn2, int *pOut, UINT nCount, const float _f2ic)
//--------------------------------------------------------------------------------------------------------
{
	_asm {
	movd mm0, _f2ic
	mov eax, pIn1
	mov ebx, pIn2
	mov edx, pOut
	mov ecx, nCount
	punpckldq mm0, mm0
	inc ecx
	shr ecx, 1
	sub edx, 16
mainloop:
	movq mm1, [eax]
	movq mm2, [ebx]
	add edx, 16
	movq mm3, mm1
	punpckldq mm1, mm2
	punpckhdq mm3, mm2
	add eax, 8
	pfmul mm1, mm0
	pfmul mm3, mm0
	add ebx, 8
	pf2id mm1, mm1
	pf2id mm3, mm3
	dec ecx
	movq qword ptr [edx], mm1
	movq qword ptr [edx+8], mm3
	jnz mainloop
	emms
	}
}


void AMD_FloatToMonoMix(const float *pIn, int *pOut, UINT nCount, const float _f2ic)
//----------------------------------------------------------------------------------
{
	_asm {
	movd mm0, _f2ic
	mov eax, pIn
	mov edx, pOut
	mov ecx, nCount
	punpckldq mm0, mm0
	add ecx, 3
	shr ecx, 2
	sub edx, 16
mainloop:
	movq mm1, [eax]
	movq mm2, [eax+8]
	add edx, 16
	pfmul mm1, mm0
	pfmul mm2, mm0
	add eax, 16
	pf2id mm1, mm1
	pf2id mm2, mm2
	dec ecx
	movq qword ptr [edx], mm1
	movq qword ptr [edx+8], mm2
	jnz mainloop
	emms
	}
}


void AMD_MonoMixToFloat(const int *pSrc, float *pOut, UINT nCount, const float _i2fc)
//-----------------------------------------------------------------------------------
{
	_asm {
	movd mm0, _i2fc
	mov eax, pSrc
	mov edx, pOut
	mov ecx, nCount
	punpckldq mm0, mm0
	add ecx, 3
	shr ecx, 2
	sub edx, 16
mainloop:
	movq mm1, qword ptr [eax]
	movq mm2, qword ptr [eax+8]
	add edx, 16
	pi2fd mm1, mm1
	pi2fd mm2, mm2
	add eax, 16
	pfmul mm1, mm0
	pfmul mm2, mm0
	dec ecx
	movq qword ptr [edx], mm1
	movq qword ptr [edx+8], mm2
	jnz mainloop
	emms
	}
}

#endif

///////////////////////////////////////////////////////////////////////////////////////
// SSE Optimizations

#ifdef ENABLE_SSE

void SSE_StereoMixToFloat(const int *pSrc, float *pOut1, float *pOut2, UINT nCount, const float _i2fc)
//----------------------------------------------------------------------------------------------------
{
	_asm {
	movss xmm0, _i2fc
	mov edx, pSrc
	mov eax, pOut1
	mov ebx, pOut2
	mov ecx, nCount
	shufps xmm0, xmm0, 0x00
	xorps xmm1, xmm1
	xorps xmm2, xmm2
	inc ecx
	shr ecx, 1
mainloop:
	cvtpi2ps xmm1, [edx]
	cvtpi2ps xmm2, [edx+8]
	add eax, 8
	add ebx, 8
	movlhps xmm1, xmm2
	mulps xmm1, xmm0
	add edx, 16
	shufps xmm1, xmm1, 0xD8
	dec ecx
	movlps qword ptr [eax-8], xmm1
	movhps qword ptr [ebx-8], xmm1
	jnz mainloop
	}
}


void SSE_MonoMixToFloat(const int *pSrc, float *pOut, UINT nCount, const float _i2fc)
//-----------------------------------------------------------------------------------
{
	_asm {
	movss xmm0, _i2fc
	mov edx, pSrc
	mov eax, pOut
	mov ecx, nCount
	shufps xmm0, xmm0, 0x00
	xorps xmm1, xmm1
	xorps xmm2, xmm2
	add ecx, 3
	shr ecx, 2
mainloop:
	cvtpi2ps xmm1, [edx]
	cvtpi2ps xmm2, [edx+8]
	add eax, 16
	movlhps xmm1, xmm2
	mulps xmm1, xmm0
	add edx, 16
	dec ecx
	movups [eax-16], xmm1
	jnz mainloop
	}
}

#endif





// Convert floating-point mix to integer
void X86_FloatToStereoMix(const float *pIn1, const float *pIn2, int *pOut, UINT nCount, const float _f2ic)
//--------------------------------------------------------------------------------------------------------
{
	_asm {
	mov esi, pIn1
	mov ebx, pIn2
	mov edi, pOut
	mov ecx, nCount
	fld _f2ic
mainloop:
	fld dword ptr [ebx]
	add edi, 8
	fld dword ptr [esi]
	add ebx, 4
	add esi, 4
	fmul st(0), st(2)
	fistp dword ptr [edi-8]
	fmul st(0), st(1)
	fistp dword ptr [edi-4]
	dec ecx
	jnz mainloop
	fstp st(0)
	}
}

// Convert integer mix to floating-point
void X86_StereoMixToFloat(const int *pSrc, float *pOut1, float *pOut2, UINT nCount, const float _i2fc)
//----------------------------------------------------------------------------------------------------
{
	_asm {
	mov esi, pSrc
	mov edi, pOut1
	mov ebx, pOut2
	mov ecx, nCount
	fld _i2fc
mainloop:
	fild dword ptr [esi]
	fild dword ptr [esi+4]
	add ebx, 4
	add edi, 4
	fmul st(0), st(2)
	add esi, 8
	fstp dword ptr [ebx-4]
	fmul st(0), st(1)
	fstp dword ptr [edi-4]
	dec ecx
	jnz mainloop
	fstp st(0)
	}
}


void X86_FloatToMonoMix(const float *pIn, int *pOut, UINT nCount, const float _f2ic)
//----------------------------------------------------------------------------------
{
	_asm {
	mov edx, pIn
	mov eax, pOut
	mov ecx, nCount
	fld _f2ic
	sub eax, 4
R2I_Loop:
	fld DWORD PTR [edx]
	add eax, 4
	fmul ST(0), ST(1)
	dec ecx
	lea edx, [edx+4]
	fistp DWORD PTR [eax]
	jnz R2I_Loop
	fstp st(0)
	}
}


void X86_MonoMixToFloat(const int *pSrc, float *pOut, UINT nCount, const float _i2fc)
//-----------------------------------------------------------------------------------
{
	_asm {
	mov edx, pOut
	mov eax, pSrc
	mov ecx, nCount
	fld _i2fc
	sub edx, 4
I2R_Loop:
	fild DWORD PTR [eax]
	add edx, 4
	fmul ST(0), ST(1)
	dec ecx
	lea eax, [eax+4]
	fstp DWORD PTR [edx]
	jnz I2R_Loop
	fstp st(0)
	}
}
