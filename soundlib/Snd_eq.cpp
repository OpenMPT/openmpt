/*
 * snd_eq.cpp
 * ----------
 * Purpose: Mixing code for equalizer.
 * Notes  : Ugh... This should really be removed at some point.
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "sndfile.h"
#include "../mptrack/TrackerSettings.h"

#define EQ_BANDWIDTH	2.0
#define EQ_ZERO			0.000001
#define REAL			float

extern REAL MixFloatBuffer[];

extern void X86_StereoMixToFloat(const int *pSrc, float *pOut1, float *pOut2, UINT nCount, const float _i2fc);
extern void X86_FloatToStereoMix(const float *pIn1, const float *pIn2, int *pOut, UINT nCount, const float _f2ic);
extern void X86_MonoMixToFloat(const int *pSrc, float *pOut, UINT nCount, const float _i2fc);
extern void X86_FloatToMonoMix(const float *pIn, int *pOut, UINT nCount, const float _f2ic);

#ifdef ENABLE_SSE
extern void SSE_MonoMixToFloat(const int *pSrc, float *pOut, UINT nCount, const float _i2fc);
#endif
#ifdef ENABLE_AMD
extern void AMD_MonoMixToFloat(const int *pSrc, float *pOut, UINT nCount, const float _i2fc);
extern void AMD_FloatToMonoMix(const float *pIn, int *pOut, UINT nCount, const float _f2ic);
#endif




#pragma pack(push, 4)
typedef struct _EQBANDSTRUCT
{
	REAL a0, a1, a2, b1, b2;
	REAL x1, x2, y1, y2;
	REAL Gain, CenterFrequency;
	BOOL bEnable;
} EQBANDSTRUCT, *PEQBANDSTRUCT;
#pragma pack(pop)

UINT gEqLinearToDB[33] =
{
	16, 19, 22, 25, 28, 31, 34, 37,
	40, 43, 46, 49, 52, 55, 58, 61,
	64, 76, 88, 100, 112, 124, 136, 148,
	160, 172, 184, 196, 208, 220, 232, 244, 256
};


static REAL f2ic = (REAL)(1 << 28);
static REAL i2fc = (REAL)(1.0 / (1 << 28));

static EQBANDSTRUCT gEQ[MAX_EQ_BANDS*2] =
{
	// Default: Flat EQ
	{0,0,0,0,0, 0,0,0,0, 1,   120, FALSE},
	{0,0,0,0,0, 0,0,0,0, 1,   600, FALSE},
	{0,0,0,0,0, 0,0,0,0, 1,  1200, FALSE},
	{0,0,0,0,0, 0,0,0,0, 1,  3000, FALSE},
	{0,0,0,0,0, 0,0,0,0, 1,  6000, FALSE},
	{0,0,0,0,0, 0,0,0,0, 1, 10000, FALSE},
	{0,0,0,0,0, 0,0,0,0, 1,   120, FALSE},
	{0,0,0,0,0, 0,0,0,0, 1,   600, FALSE},
	{0,0,0,0,0, 0,0,0,0, 1,  1200, FALSE},
	{0,0,0,0,0, 0,0,0,0, 1,  3000, FALSE},
	{0,0,0,0,0, 0,0,0,0, 1,  6000, FALSE},
	{0,0,0,0,0, 0,0,0,0, 1, 10000, FALSE},
};

#define ASM_EQ
#ifdef ASM_EQ

#pragma warning(disable:4100)

#define PBS_A0	DWORD PTR [eax]
#define PBS_A1	DWORD PTR [eax+4]
#define PBS_A2	DWORD PTR [eax+8]
#define PBS_B1	DWORD PTR [eax+12]
#define PBS_B2	DWORD PTR [eax+16]
#define PBS_X1	DWORD PTR [eax+20]
#define PBS_X2	DWORD PTR [eax+24]
#define PBS_Y1	DWORD PTR [eax+28]
#define PBS_Y2	DWORD PTR [eax+32]

void __cdecl EQFilter(EQBANDSTRUCT *pbs, REAL *pbuffer, UINT nCount)
//------------------------------------------------------------------
{
	_asm {
	mov eax, pbs		// eax = pbs
	mov edx, nCount		// edx = nCount
	mov ecx, pbuffer	// ecx = pbuffer
	fld		PBS_Y2		// ST(3)=y2
	fld		PBS_Y1		// ST(2)=y1
	fld		PBS_X2		// ST(1)=x2
	fld		PBS_X1		// ST(0)=x1
EQ_Loop:
	fld		DWORD PTR [ecx]		// ST(0):x ST(1):x1 ST(2):x2 ST(3):y1 ST(4):y2
	fld		PBS_A0				// ST(0):a0 ST(1):x ST(2):x1 ST(3):x2 ST(4):y1 ST(5):y2
	fmul	ST(0), ST(1)		// ST(0):a0*x
	fld		PBS_A1				// ST(0):a1 ST(1):a0*x ST(2):x ST(3):x1 ST(4):x2 ST(5):y1 ST(6):y2
	fmul	ST(0), ST(3)		// ST(0):a1*x1
	add		ecx, 4

	faddp	ST(1), ST(0)
	fld		PBS_A2
	fmul	ST(0), ST(4)
	faddp	ST(1), ST(0)
	fld		PBS_B1
	fmul	ST(0), ST(5)
	faddp	ST(1), ST(0)
	fld		PBS_B2
	fmul	ST(0), ST(6)
	sub     edx, 1
	faddp	ST(1), ST(0)
	fst		DWORD PTR [ecx-4]		// *pbuffer = a0*x+a1*x1+a2*x2+b1*y1+b2*y2
	// Here, ST(0)=y ST(1)=x ST(2)=x1 ST(3)=x2 ST(4)=y1 ST(5)=y2
	fxch	ST(4)	// y1=y
	fstp	ST(5)	// y2=y1
	// Here, ST(0)=x ST(1)=x1 ST(2)=x2 ST(3)=y1 ST(4)=y2
	fxch	ST(1)	// x1=x
	fstp	ST(2)	// x2=x1
	jnz		EQ_Loop
	// Store x1,y1,x2,y2 and pop FPU stack
	fstp	PBS_X1
	fstp	PBS_X2
	fstp	PBS_Y1
	fstp	PBS_Y2
	}
}


void AMD_StereoEQ(EQBANDSTRUCT *pbl, EQBANDSTRUCT *pbr, REAL *pbuffer, UINT nCount)
//---------------------------------------------------------------------------------
{
#ifdef ENABLE_AMD
	float tmp[16];

	_asm {
	mov eax, pbl
	mov edx, pbr
	mov ebx, pbuffer
	mov ecx, nCount
	lea edi, [tmp+8]
	and edi, 0xfffffff8
	movd mm7, [eax+EQBANDSTRUCT.a0]
	movd mm0, [edx+EQBANDSTRUCT.a0]
	movd mm6, [eax+EQBANDSTRUCT.a1]
	movd mm1, [edx+EQBANDSTRUCT.a1]
	punpckldq mm7, mm0
	punpckldq mm6, mm1
	movq [edi], mm7						// [edi] = a0
	movq [edi+8], mm6					// [edi+8] = a1
	movd mm5, [eax+EQBANDSTRUCT.a2]
	movd mm0, [edx+EQBANDSTRUCT.a2]
	movd mm4, [eax+EQBANDSTRUCT.b1]
	movd mm1, [edx+EQBANDSTRUCT.b1]
	movd mm3, [eax+EQBANDSTRUCT.b2]
	movd mm2, [edx+EQBANDSTRUCT.b2]
	punpckldq mm5, mm0
	punpckldq mm4, mm1
	punpckldq mm3, mm2
	movq [edi+16], mm5					// [edi+16] = a2
	movq [edi+24], mm4					// [edi+24] = b1
	movq [edi+32], mm3					// [edi+32] = b2
	movd mm4, [eax+EQBANDSTRUCT.x1]
	movd mm0, [edx+EQBANDSTRUCT.x1]
	movd mm5, [eax+EQBANDSTRUCT.x2]
	movd mm1, [edx+EQBANDSTRUCT.x2]
	punpckldq mm4, mm0					// mm4 = x1
	punpckldq mm5, mm1					// mm5 = x2
	movd mm6, [eax+EQBANDSTRUCT.y1]
	movd mm2, [edx+EQBANDSTRUCT.y1]
	movd mm7, [eax+EQBANDSTRUCT.y2]
	movd mm3, [edx+EQBANDSTRUCT.y2]
	punpckldq mm6, mm2					// mm6 = y1
	punpckldq mm7, mm3					// mm7 = y2
mainloop:
	movq mm0, [ebx]
	movq mm3, [edi+8]
	add ebx, 8
	movq mm1, [edi+16]
	pfmul mm3, mm4						// x1 * a1
	movq mm2, [edi+32]
	pfmul mm1, mm5						// x2 * a2
	movq mm5, mm4						// x2 = x1
	pfmul mm2, mm7						// y2 * b2
	movq mm7, mm6						// y2 = y1
	pfmul mm6, [edi+24]					// y1 * b1
	movq mm4, mm0						// x1 = x
	pfmul mm0, [edi]					// x * a0
	pfadd mm6, mm1						// x2*a2 + y1*b1
	pfadd mm6, mm2						// x2*a2 + y1*b1 + y2*b2
	pfadd mm6, mm3						// x1*a1 + x2*a2 + y1*b1 + y2*b2
	pfadd mm6, mm0						// x*a0 + x1*a1 + x2*a2 + y1*b1 + y2*b2
	dec ecx
	movq [ebx-8], mm6
	jnz mainloop
	movd [eax+EQBANDSTRUCT.x1], mm4
	punpckhdq mm4, mm4
	movd [eax+EQBANDSTRUCT.x2], mm5
	punpckhdq mm5, mm5
	movd [eax+EQBANDSTRUCT.y1], mm6
	punpckhdq mm6, mm6
	movd [eax+EQBANDSTRUCT.y2], mm7
	punpckhdq mm7, mm7
	movd [edx+EQBANDSTRUCT.x1], mm4
	movd [edx+EQBANDSTRUCT.x2], mm5
	movd [edx+EQBANDSTRUCT.y1], mm6
	movd [edx+EQBANDSTRUCT.y2], mm7
	emms
	}
#endif
}


void SSE_StereoEQ(EQBANDSTRUCT *pbl, EQBANDSTRUCT *pbr, REAL *pbuffer, UINT nCount)
//---------------------------------------------------------------------------------
{
#ifdef ENABLE_SSE
	static const float gk1 = 1.0f;
	_asm {
	mov eax, pbl
	mov edx, pbr
	mov ebx, pbuffer
	mov ecx, nCount
	movss xmm0, [eax+EQBANDSTRUCT.Gain]
	movss xmm1, gk1
	comiss xmm0, xmm1
	jne doeq
	movss xmm0, [edx+EQBANDSTRUCT.Gain]
	comiss xmm0, xmm1
	je done
doeq:
	test ecx, ecx
	jz done
	movss xmm6, [eax+EQBANDSTRUCT.a1]
	movss xmm7, [eax+EQBANDSTRUCT.a2]
	movss xmm4, [eax+EQBANDSTRUCT.x1]
	movss xmm5, [eax+EQBANDSTRUCT.x2]
	movlhps xmm6, xmm7		// xmm6 = [  0 | a2 |  0 | a1 ]
	movlhps xmm4, xmm5		// xmm4 = [  0 | x2 |  0 | x1 ]
	movss xmm2, [edx+EQBANDSTRUCT.a1]
	movss xmm3, [edx+EQBANDSTRUCT.a2]
	movss xmm0, [edx+EQBANDSTRUCT.x1]
	movss xmm1, [edx+EQBANDSTRUCT.x2]
	movlhps xmm2, xmm3		// xmm2 = [ 0 | a'2 | 0 | a'1 ]
	movlhps xmm0, xmm1		// xmm0 = [ 0 | x'2 | 0 | x'1 ]
	shufps xmm6, xmm2, 0x88	// xmm6 = [ a'2 | a'1 | a2 | a1 ]
	shufps xmm4, xmm0, 0x88 // xmm4 = [ x'2 | x'1 | x2 | x1 ]
	shufps xmm6, xmm6, 0xD8	// xmm6 = [ a'2 | a2 | a'1 | a1 ]
	shufps xmm4, xmm4, 0xD8	// xmm4 = [ x'2 | x2 | x'1 | x1 ]
	movss xmm7, [eax+EQBANDSTRUCT.b1]
	movss xmm0, [eax+EQBANDSTRUCT.b2]
	movss xmm2, [edx+EQBANDSTRUCT.b1]
	movss xmm1, [edx+EQBANDSTRUCT.b2]
	movlhps xmm7, xmm0		// xmm7 = [ 0 |  b2 | 0 |  b1 ]
	movlhps xmm2, xmm1		// xmm2 = [  0 | b'2 |  0 | b'1 ]
	shufps xmm7, xmm2, 0x88	// xmm7 = [ b'2 | b'1 | b2 | b1 ]
	shufps xmm7, xmm7, 0xD8 // xmm7 = [ b'2 | b2 | b'1 | b1 ]
	movss xmm5, [eax+EQBANDSTRUCT.y1]
	movss xmm1, [eax+EQBANDSTRUCT.y2]
	movss xmm3, [edx+EQBANDSTRUCT.y1]
	movss xmm0, [edx+EQBANDSTRUCT.y2]
	movlhps xmm5, xmm1		// xmm5 = [  0 |  y2 |  0 |  y1 ]
	movlhps xmm3, xmm0		// xmm3 = [  0 | y'2 |  0 | y'1 ]
	shufps xmm5, xmm3, 0x88 // xmm5 = [ y'2 | y'1 | y2 | y1 ]
	shufps xmm5, xmm5, 0xD8 // xmm5 = [ y'2 |  y2 | y'1 |  y1 ]
	movss xmm3, [eax+EQBANDSTRUCT.a0]
	movss xmm2, [edx+EQBANDSTRUCT.a0]
	shufps xmm3, xmm2, 0x88
	shufps xmm3, xmm3, 0xD8	// xmm3 = [  0 |  0 | a'0 | a0 ]
mainloop:
	movlps xmm0, qword ptr [ebx]
	add ebx, 8
	movaps xmm1, xmm5		// xmm1 = [ y2r | y2l | y1r | y1l ]
	mulps xmm1, xmm7		// xmm1 = [b2r*y2r|b2l*y2l|b1r*y1r|b1l*y1l]
	movaps xmm2, xmm4		// xmm2 = [ x2r | x2l | x1r | x1l ]
	mulps xmm2, xmm6		// xmm6 = [a2r*x2r|a2l*x2l|a1r*x1r|a1l*x1l]
	shufps xmm4, xmm0, 0x44 // xmm4 = [  xr |  xl | x1r | x1l ]
	mulps xmm0, xmm3		// xmm0 = [   0 |   0 |a0r*xr|a0l*xl]
	shufps xmm4, xmm4, 0x4E // xmm4 = [ x1r | x1l |  xr |  xl ]
	addps xmm1, xmm2		// xmm1 = [b2r*y2r+a2r*x2r|b2l*y2l+a2l*x2l|b1r*y1r+a1r*x1r|b1l*y1l+a1l*x1l]
	addps xmm1, xmm0		// xmm1 = [b2r*y2r+a2r*x2r|b2l*y2l+a2l*x2l|b1r*y1r+a1r*x1r+a0r*xr|b1l*y1l+a1l*x1l+a0l*xl]
	sub ecx, 1
	movhlps xmm0, xmm1
	addps xmm0, xmm1		// xmm0 = [  ? |  ? | yr(n) | yl(n) ]
	movlps [ebx-8], xmm0
	shufps xmm0, xmm5, 0x44
	movaps xmm5, xmm0
	jnz mainloop
	movhlps xmm0, xmm4
	movhlps xmm1, xmm5
	movss [eax+EQBANDSTRUCT.x1], xmm4
	movss [eax+EQBANDSTRUCT.x2], xmm0
	movss [eax+EQBANDSTRUCT.y1], xmm5
	movss [eax+EQBANDSTRUCT.y2], xmm1
	shufps xmm4, xmm4, 0x01
	shufps xmm0, xmm0, 0x01
	shufps xmm5, xmm5, 0x01
	shufps xmm1, xmm1, 0x01
	movss [edx+EQBANDSTRUCT.x1], xmm4
	movss [edx+EQBANDSTRUCT.x2], xmm0
	movss [edx+EQBANDSTRUCT.y1], xmm5
	movss [edx+EQBANDSTRUCT.y2], xmm1
done:;
	}
#endif SSE_SPECIFIC
}

#pragma warning(default:4100)

#else

void EQFilter(EQBANDSTRUCT *pbs, REAL *pbuffer, UINT nCount)
//----------------------------------------------------------
{
	for (UINT i=0; i<nCount; i++)
	{
		REAL x = pbuffer[i];
		REAL y = pbs->a1 * pbs->x1 + pbs->a2 * pbs->x2 + pbs->a0 * x + pbs->b1 * pbs->y1 + pbs->b2 * pbs->y2;
		pbs->x2 = pbs->x1;
		pbs->y2 = pbs->y1;
		pbs->x1 = x;
		pbuffer[i] = y;
		pbs->y1 = y;
	}
}

#endif


void CSoundFile::EQMono(int *pbuffer, UINT nCount)
//------------------------------------------------
{
	MonoMixToFloat(pbuffer, MixFloatBuffer, nCount);
	for (UINT b=0; b<MAX_EQ_BANDS; b++)
	{
		if ((gEQ[b].bEnable) && (gEQ[b].Gain != 1.0f)) EQFilter(&gEQ[b], MixFloatBuffer, nCount);
	}
	FloatToMonoMix(MixFloatBuffer, pbuffer, nCount);
}


void CSoundFile::EQStereo(int *pbuffer, UINT nCount)
//--------------------------------------------------
{

#ifdef ENABLE_SSE
#ifdef ENABLE_MMX

	// Still allow the check, because the user can turn this on/off
	
	if ((gdwSysInfo & SYSMIX_SSE) && (gdwSoundSetup & SNDMIX_ENABLEMMX))
    {
		int sse_state, sse_eqstate;
		SSE_MonoMixToFloat(pbuffer, MixFloatBuffer, nCount*2, m_pConfig->getIntToFloat());

		_asm stmxcsr sse_state;
		sse_eqstate = sse_state | 0xFF80;
		_asm ldmxcsr sse_eqstate;
		for (UINT b=0; b<MAX_EQ_BANDS; b++)
		{
			if ((gEQ[b].bEnable) || (gEQ[b+MAX_EQ_BANDS].bEnable))
				SSE_StereoEQ(&gEQ[b], &gEQ[b+MAX_EQ_BANDS], MixFloatBuffer, nCount);
		}
		_asm ldmxcsr sse_state;

		X86_FloatToMonoMix(MixFloatBuffer, pbuffer, nCount*2, m_pConfig->getFloatToInt());

	} else

#endif // ENABLE_SSE
#endif // ENABLE_AMD

#ifdef ENABLE_AMD
     
     // We still perform the MMX check because the user can enable/disable this

	if ((gdwSysInfo & SYSMIX_3DNOW) && (gdwSoundSetup & SNDMIX_ENABLEMMX))
	{ 
		AMD_MonoMixToFloat(pbuffer, MixFloatBuffer, nCount*2, m_pConfig->getIntToFloat());

		for (UINT b=0; b<MAX_EQ_BANDS; b++)
		{
			if (((gEQ[b].bEnable) && (gEQ[b].Gain != 1.0f))
			 || ((gEQ[b+MAX_EQ_BANDS].bEnable) && (gEQ[b+MAX_EQ_BANDS].Gain != 1.0f)))
				AMD_StereoEQ(&gEQ[b], &gEQ[b+MAX_EQ_BANDS], MixFloatBuffer, nCount);
		}

		AMD_FloatToMonoMix(MixFloatBuffer, pbuffer, nCount*2, m_pConfig->getFloatToInt());
		
	} else
#endif // ENABLE_AMD

	{	
		X86_StereoMixToFloat(pbuffer, MixFloatBuffer, MixFloatBuffer+MIXBUFFERSIZE, nCount, m_pConfig->getIntToFloat());
		
		for (UINT bl=0; bl<MAX_EQ_BANDS; bl++)
		{
			if ((gEQ[bl].bEnable) && (gEQ[bl].Gain != 1.0f)) EQFilter(&gEQ[bl], MixFloatBuffer, nCount);
		}
		for (UINT br=MAX_EQ_BANDS; br<MAX_EQ_BANDS*2; br++)
		{
			if ((gEQ[br].bEnable) && (gEQ[br].Gain != 1.0f)) EQFilter(&gEQ[br], MixFloatBuffer+MIXBUFFERSIZE, nCount);
		}

		X86_FloatToStereoMix(MixFloatBuffer, MixFloatBuffer+MIXBUFFERSIZE, pbuffer, nCount, m_pConfig->getFloatToInt());
	}
}


void CSoundFile::InitializeEQ(BOOL bReset)
//----------------------------------------
{
	REAL fMixingFreq = (REAL)gdwMixingFreq;
	// Gain = 0.5 (-6dB) .. 2 (+6dB)
	for (UINT band=0; band<MAX_EQ_BANDS*2; band++) if (gEQ[band].bEnable)
	{
		REAL k, k2, r, f;
		REAL v0, v1;
		BOOL b = bReset;

		f = gEQ[band].CenterFrequency / fMixingFreq;
		if (f > 0.45f) gEQ[band].Gain = 1;
		// if (f > 0.25) f = 0.25;
		// k = tan(PI*f);
		k = f * 3.141592654f;
		k = k + k*f;
//		if (k > (REAL)0.707) k = (REAL)0.707;
		k2 = k*k;
		v0 = gEQ[band].Gain;
		v1 = 1;
		if (gEQ[band].Gain < 1.0)
		{
			v0 *= (0.5f/EQ_BANDWIDTH);
			v1 *= (0.5f/EQ_BANDWIDTH);
		} else
		{
			v0 *= (1.0f/EQ_BANDWIDTH);
			v1 *= (1.0f/EQ_BANDWIDTH);
		}
		r = (1 + v0*k + k2) / (1 + v1*k + k2);
		if (r != gEQ[band].a0)
		{
			gEQ[band].a0 = r;
			b = TRUE;
		}
		r = 2 * (k2 - 1) / (1 + v1*k + k2);
		if (r != gEQ[band].a1)
		{
			gEQ[band].a1 = r;
			b = TRUE;
		}
		r = (1 - v0*k + k2) / (1 + v1*k + k2);
		if (r != gEQ[band].a2)
		{
			gEQ[band].a2 = r;
			b = TRUE;
		}
		r = - 2 * (k2 - 1) / (1 + v1*k + k2);
		if (r != gEQ[band].b1)
		{
			gEQ[band].b1 = r;
			b = TRUE;
		}
		r = - (1 - v1*k + k2) / (1 + v1*k + k2);
		if (r != gEQ[band].b2)
		{
			gEQ[band].b2 = r;
			b = TRUE;
		}
		if (b)
		{
			gEQ[band].x1 = 0;
			gEQ[band].x2 = 0;
			gEQ[band].y1 = 0;
			gEQ[band].y2 = 0;
		}
	} else
	{
		gEQ[band].a0 = 0;
		gEQ[band].a1 = 0;
		gEQ[band].a2 = 0;
		gEQ[band].b1 = 0;
		gEQ[band].b2 = 0;
		gEQ[band].x1 = 0;
		gEQ[band].x2 = 0;
		gEQ[band].y1 = 0;
		gEQ[band].y2 = 0;
	}
}


void CSoundFile::SetEQGains(const UINT *pGains, UINT nGains, const UINT *pFreqs, BOOL bReset)
//-------------------------------------------------------------------------------------------
{
	for (UINT i=0; i<MAX_EQ_BANDS; i++)
	{
		REAL g, f = 0;
		if (i < nGains)
		{
			UINT n = pGains[i];
			if (n > 32) n = 32;
			g = ((REAL)gEqLinearToDB[n]) / 64.0f;
			if (pFreqs) f = (REAL)(int)pFreqs[i];
		} else
		{
			g = 1;
		}
		gEQ[i].Gain = g;
		gEQ[i].CenterFrequency = f;
		gEQ[i+MAX_EQ_BANDS].Gain = g;
		gEQ[i+MAX_EQ_BANDS].CenterFrequency = f;
		if (f > 20.0f)
		{
			gEQ[i].bEnable = TRUE;
			gEQ[i+MAX_EQ_BANDS].bEnable = TRUE;
		} else
		{
			gEQ[i].bEnable = FALSE;
			gEQ[i+MAX_EQ_BANDS].bEnable = FALSE;
		}
	}
	InitializeEQ(bReset);
}
