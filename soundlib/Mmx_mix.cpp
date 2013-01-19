/*
 * MMX_Mix.cpp
 * -----------
 * Purpose: Accelerated mixer code for rendering samples.
 * Notes  : 
 *          x86 ( AMD/INTEL ) based low level based mixing functions:
 *          This file contains critical code. The basic X86 functions are
 *          defined at the bottom of the file. #define's are used to isolate
 *          the different flavours of functionality:
 *          ENABLE_MMX, ENABLE_AMDNOW, ENABLE_SSE flags must be set to
 *          to compile the optimized sections of the code. In both cases the 
 *          X86_xxxxxx functions will compile.
 *
 *          NOTE: Resonant filter mixing has been changed to use floating point
 *          precision. The MMX code hasn't been updated for this, so the filtered
 *          MMX functions mustn't be used anymore!
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "sndfile.h"
#include "../common/Reporting.h"

extern short int gFastSinc[];
extern short int gKaiserSinc[];
extern short int gDownsample13x[];
extern short int gDownsample2x[];

#define PROCSUPPORT_CPUID	0x01
#define PROCSUPPORT_MMX		0x02
#define PROCSUPPORT_MMXEX	0x04
#define PROCSUPPORT_3DNOW	0x08
#define PROCSUPPORT_SSE		0x10

// Byte offsets into the ModChannel structure
#define CHNOFS_PCURRENTSAMPLE	ModChannel.pCurrentSample	// 0
#define CHNOFS_NPOS				ModChannel.nPos				// 4
#define CHNOFS_NPOSLO			ModChannel.nPosLo			// 8
#define CHNOFS_NINC				ModChannel.nInc				// 12
#define CHNOFS_NRIGHTVOL		ModChannel.nRightVol		// 16
#define CHNOFS_NLEFTVOL			ModChannel.nLeftVol			// 20
#define CHNOFS_NRIGHTRAMP		ModChannel.nRightRamp		// 24
#define CHNOFS_NLEFTRAMP		ModChannel.nLeftRamp		// 28


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
	if (sizeof(ModChannel) & 0x1F)
	{
		CHAR s[64];
		wsprintf(s, "ModChannel struct not aligned: sizeof(ModChannel) = %d", sizeof(ModChannel));
		Reporting::Warning(s);
	}
	DWORD dwFastSinc = (DWORD)(LPVOID)gFastSinc;
	if (dwFastSinc & 7)
	{
		CHAR s[64];
		wsprintf(s, "gFastSinc is not aligned (%08X)!", dwFastSinc);
		Reporting::Warning(s);
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
