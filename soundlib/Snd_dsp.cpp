/*
 * This program is  free software; you can redistribute it  and modify it
 * under the terms of the GNU  General Public License as published by the
 * Free Software Foundation; either version 2  of the license or (at your
 * option) any later version.
 *
 * Authors: Olivier Lapicque <olivierl@jps.net>
*/

#include "stdafx.h"
#include "sndfile.h"

// Bass Expansion
#define DEFAULT_XBASS_RANGE		14	// (x+2)*20 Hz (320Hz)
#define DEFAULT_XBASS_DEPTH		6	// 1+(3>>(x-4)) (+6dB)

// Buffer Sizes
#define XBASSBUFFERSIZE			64		// 2 ms at 50KHz
#define SURROUNDBUFFERSIZE		2048	// 50ms @ 48kHz


// DSP Effects: PUBLIC members
UINT CSoundFile::m_nXBassDepth = DEFAULT_XBASS_DEPTH;
UINT CSoundFile::m_nXBassRange = DEFAULT_XBASS_RANGE;
UINT CSoundFile::gnReverbType = 0;
UINT CSoundFile::m_nReverbDepth = 8; // 50%
UINT CSoundFile::m_nProLogicDepth = 12;
UINT CSoundFile::m_nProLogicDelay = 20;

////////////////////////////////////////////////////////////////////
// DSP Effects internal state

// Noise Reduction: simple low-pass filter
static LONG nLeftNR = 0;
static LONG nRightNR = 0;

// Surround Encoding: 1 delay line + low-pass filter + high-pass filter
static LONG nSurroundSize = 0;
static LONG nSurroundPos = 0;
static LONG nDolbyDepth = 0;
// Surround Biquads
static LONG nDolbyHP_Y1 = 0;
static LONG nDolbyHP_X1 = 0;
static LONG nDolbyLP_Y1 = 0;
static LONG nDolbyHP_B0 = 0;
static LONG nDolbyHP_B1 = 0;
static LONG nDolbyHP_A1 = 0;
static LONG nDolbyLP_B0 = 0;
static LONG nDolbyLP_B1 = 0;
static LONG nDolbyLP_A1 = 0;

// Bass Expansion: low-pass filter
static LONG nXBassFlt_Y1 = 0;
static LONG nXBassFlt_X1 = 0;
static LONG nXBassFlt_B0 = 0;
static LONG nXBassFlt_B1 = 0;
static LONG nXBassFlt_A1 = 0;

// DC Removal Biquad
static LONG nDCRFlt_Y1l = 0;
static LONG nDCRFlt_X1l = 0;
static LONG nDCRFlt_Y1r = 0;
static LONG nDCRFlt_X1r = 0;


static LONG SurroundBuffer[SURROUNDBUFFERSIZE];


// Access the main temporary mix buffer directly: avoids an extra pointer
extern int MixSoundBuffer[MIXBUFFERSIZE*4];
extern int MixRearBuffer[MIXBUFFERSIZE*2];

extern VOID InitializeReverb(BOOL bReset);
extern VOID ProcessReverb(UINT nSamples);
extern VOID MPPASMCALL X86_InitMixBuffer(int *pBuffer, UINT nSamples);
extern VOID MPPASMCALL X86_StereoFill(int *pBuffer, UINT nSamples, LPLONG lpROfs, LPLONG lpLOfs);
extern VOID MPPASMCALL X86_StereoDCRemoval(int *, UINT count);
extern VOID MPPASMCALL X86_MonoDCRemoval(int *, UINT count);

///////////////////////////////////////////////////////////////////////////////////
//
// Biquad setup
//

// returns sqrt(x)
__inline float sqrt(FLOAT x)
{
	float result;
	_asm {
	fld x
	fsqrt
	fstp result
	}
	return result;
}

// sin(x) - sine of x (radians)
__inline float sin(float x)
{
	__asm {
	fld x
	fsin
	fstp x
	}
	return x;
}


#define PI	3.14159265358979323f
inline FLOAT Sgn(FLOAT x) { return (x >= 0) ? 1.0f : -1.0f; }
VOID ShelfEQ(LONG scale,
			 LONG *outA1, LONG *outB0, LONG *outB1,
			 LONG F_c, LONG F_s, FLOAT gainDC, FLOAT gainFT, FLOAT gainPI)
{
	FLOAT a1, b0, b1;
	FLOAT gainFT2, gainDC2, gainPI2;
	FLOAT alpha, beta0, beta1, rho;
	FLOAT wT, quad;
        
	_asm {
	// wT = PI*Fc/Fs
	fild F_c
	fldpi
	fmulp ST(1), ST(0)
	fild F_s
	fdivp ST(1), ST(0)			
	fstp wT
	// gain^2
	fld gainDC
	fld gainFT
	fld gainPI
	fmul ST(0), ST(0)
	fstp gainPI2
	fmul ST(0), ST(0)
	fstp gainFT2
	fmul ST(0), ST(0)
	fstp gainDC2
	}

	quad = gainPI2 + gainDC2 - (gainFT2*2);

	alpha = 0;
 
	if (quad != 0)
	{
		FLOAT lambda = (gainPI2 - gainDC2) / quad;
		alpha  = (FLOAT)(lambda - Sgn(lambda)*sqrt(lambda*lambda - 1.0f));
	}
 
	beta0 = 0.5f * ((gainDC + gainPI) + (gainDC - gainPI) * alpha);
	beta1 = 0.5f * ((gainDC - gainPI) + (gainDC + gainPI) * alpha);
	rho   = (FLOAT)((sin((wT*0.5f) - (PI/4.0f))) / (sin((wT*0.5f) + (PI/4.0f))));
 
	quad  = 1.0f / (1.0f + rho*alpha);
    
	b0 = ((beta0 + rho*beta1) * quad);
	b1 = ((beta1 + rho*beta0) * quad);
	a1 = - ((rho + alpha) * quad);

	_asm {
	fild scale
	fld a1
	mov eax, outA1
	fmul ST(0), ST(1)
	fistp dword ptr [eax]
	fld b0
	mov eax, outB0
	fmul ST(0), ST(1)
	fistp dword ptr [eax]
	fld b1
	mov eax, outB1
	fmul ST(0), ST(1)
	fistp dword ptr [eax]
	fstp rho
	}
}


void CSoundFile::InitializeDSP(BOOL bReset)
//-----------------------------------------
{
	if (gnReverbType >= NUM_REVERBTYPES) gnReverbType = 0;
	if (!m_nProLogicDelay) m_nProLogicDelay = 20;
	if (bReset)
	{
		// Noise Reduction
		nLeftNR = nRightNR = 0;
	}
	// Pro-Logic Surround
	nSurroundPos = nSurroundSize = 0;
	if (gdwSoundSetup & SNDMIX_SURROUND)
	{
		memset(SurroundBuffer, 0, sizeof(SurroundBuffer));
		nSurroundSize = (gdwMixingFreq * m_nProLogicDelay) / 1000;
		if (nSurroundSize > SURROUNDBUFFERSIZE) nSurroundSize = SURROUNDBUFFERSIZE;
		nDolbyDepth = m_nProLogicDepth;
		if (nDolbyDepth < 1) nDolbyDepth = 1;
		if (nDolbyDepth > 16) nDolbyDepth = 16;
		// Setup biquad filters
		ShelfEQ(1024, &nDolbyHP_A1, &nDolbyHP_B0, &nDolbyHP_B1, 200, gdwMixingFreq, 0, 0.5f, 1);
		ShelfEQ(1024, &nDolbyLP_A1, &nDolbyLP_B0, &nDolbyLP_B1, 7000, gdwMixingFreq, 1, 0.75f, 0);
		nDolbyHP_X1 = nDolbyHP_Y1 = 0;
		nDolbyLP_Y1 = 0;
		// Surround Level
		nDolbyHP_B0 = (nDolbyHP_B0 * nDolbyDepth) >> 5;
		nDolbyHP_B1 = (nDolbyHP_B1 * nDolbyDepth) >> 5;
		// +6dB
		nDolbyLP_B0 *= 2;
		nDolbyLP_B1 *= 2;
	}
	// Reverb Setup
#ifndef NO_REVERB
	InitializeReverb(bReset);
#endif
	// Bass Expansion Reset
	if (gdwSoundSetup & SNDMIX_MEGABASS)
	{
		LONG a1 = 0, b0 = 1024, b1 = 0;
		int nXBassCutOff = 50 + (m_nXBassRange+2) * 20;
		int nXBassGain = m_nXBassDepth;
		if (nXBassGain < 2) nXBassGain = 2;
		if (nXBassGain > 8) nXBassGain = 8;
		if (nXBassCutOff < 60) nXBassCutOff = 60;
		if (nXBassCutOff > 600) nXBassCutOff = 600;
		ShelfEQ(1024, &a1, &b0, &b1, nXBassCutOff, gdwMixingFreq,
				1.0f + (1.0f/16.0f) * (0x300 >> nXBassGain),
				1.0f,
				0.0000001f);
		if (nXBassGain > 5)
		{
			b0 >>= (nXBassGain-5);
			b1 >>= (nXBassGain-5);
		}
		nXBassFlt_A1 = a1;
		nXBassFlt_B0 = b0;
		nXBassFlt_B1 = b1;
		//Log("b0=%d b1=%d a1=%d\n", b0, b1, a1);
	}
	if (bReset)
	{
		nXBassFlt_X1 = 0;
		nXBassFlt_Y1 = 0;
		nDCRFlt_X1l = 0;
		nDCRFlt_X1r = 0;
		nDCRFlt_Y1l = 0;
		nDCRFlt_Y1r = 0;
	}
}


// 2-channel surround
static void ProcessStereoSurround(int count)
//------------------------------------------
{
	int *pr = MixSoundBuffer, hy1 = nDolbyHP_Y1;
	for (int r=count; r; r--)
	{
		// Delay
		int secho = SurroundBuffer[nSurroundPos];
		SurroundBuffer[nSurroundPos] = (pr[0]+pr[1]+256) >> 9;
		// High-pass
		int v0 = (nDolbyHP_B0 * secho + nDolbyHP_B1 * nDolbyHP_X1 + nDolbyHP_A1 * hy1) >> 10;
		nDolbyHP_X1 = secho;
		// Low-pass
		int v = (nDolbyLP_B0 * v0 + nDolbyLP_B1 * hy1 + nDolbyLP_A1 * nDolbyLP_Y1) >> (10-8);
		hy1 = v0;
		nDolbyLP_Y1 = v >> 8;
		// Add echo
		pr[0] += v;
		pr[1] -= v;
		if (++nSurroundPos >= nSurroundSize) nSurroundPos = 0;
		pr += 2;
	}
	nDolbyHP_Y1 = hy1;
}


#ifndef FASTSOUNDLIB
// 4-channels surround
static void ProcessQuadSurround(int count)
//----------------------------------------
{
	int *pr = MixSoundBuffer, hy1 = nDolbyHP_Y1;
	for (int r=count; r; r--)
	{
		int vl = pr[0] >> 1;
		int vr = pr[1] >> 1;
		pr[(UINT)(MixRearBuffer-MixSoundBuffer)] += vl;
		pr[((UINT)(MixRearBuffer-MixSoundBuffer))+1] += vr;
		// Delay
		int secho = SurroundBuffer[nSurroundPos];
		SurroundBuffer[nSurroundPos] = (vr+vl+256) >> 9;
		// High-pass
		int v0 = (nDolbyHP_B0 * secho + nDolbyHP_B1 * nDolbyHP_X1 + nDolbyHP_A1 * hy1) >> 10;
		nDolbyHP_X1 = secho;
		// Low-pass
		int v = (nDolbyLP_B0 * v0 + nDolbyLP_B1 * hy1 + nDolbyLP_A1 * nDolbyLP_Y1) >> (10-8);
		hy1 = v0;
		nDolbyLP_Y1 = v >> 8;
		// Add echo
		pr[(UINT)(MixRearBuffer-MixSoundBuffer)] += v;
		pr[((UINT)(MixRearBuffer-MixSoundBuffer))+1] += v;
		if (++nSurroundPos >= nSurroundSize) nSurroundPos = 0;
		pr += 2;
	}
	nDolbyHP_Y1 = hy1;
}
#endif


void CSoundFile::ProcessStereoDSP(int count)
//------------------------------------------
{
	// Dolby Pro-Logic Surround
	if (gdwSoundSetup & SNDMIX_SURROUND)
	{
#ifndef FASTSOUNDLIB
		if (gnChannels > 2) ProcessQuadSurround(count); else
#endif
		ProcessStereoSurround(count);
	}
	// DC Removal
	if (gdwSoundSetup & SNDMIX_MEGABASS)
	{
		X86_StereoDCRemoval(MixSoundBuffer, count);
	}
	// Bass Expansion
	if (gdwSoundSetup & SNDMIX_MEGABASS)
	{
		int *px = MixSoundBuffer;
		int x1 = nXBassFlt_X1;
		int y1 = nXBassFlt_Y1;
		for (int x=count; x; x--)
		{
			int x_m = (px[0]+px[1]+0x100)>>9;

			y1 = (nXBassFlt_B0 * x_m + nXBassFlt_B1 * x1 + nXBassFlt_A1 * y1) >> (10-8);
			x1 = x_m;
			px[0] += y1;
			px[1] += y1;
			y1 = (y1+0x80) >> 8;
			px += 2;
		}
		nXBassFlt_X1 = x1;
		nXBassFlt_Y1 = y1;
	}
	// Noise Reduction
	if (gdwSoundSetup & SNDMIX_NOISEREDUCTION)
	{
		int n1 = nLeftNR, n2 = nRightNR;
		int *pnr = MixSoundBuffer;
		for (int nr=count; nr; nr--)
		{
			int vnr = pnr[0] >> 1;
			pnr[0] = vnr + n1;
			n1 = vnr;
			vnr = pnr[1] >> 1;
			pnr[1] = vnr + n2;
			n2 = vnr;
			pnr += 2;
		}
		nLeftNR = n1;
		nRightNR = n2;
	}
}


void CSoundFile::ProcessMonoDSP(int count)
//----------------------------------------
{
	// DC Removal
	if (gdwSoundSetup & SNDMIX_MEGABASS)
	{
		X86_MonoDCRemoval(MixSoundBuffer, count);
	}
	// Bass Expansion
	if (gdwSoundSetup & SNDMIX_MEGABASS)
	{
		int *px = MixSoundBuffer;
		int x1 = nXBassFlt_X1;
		int y1 = nXBassFlt_Y1;
		for (int x=count; x; x--)
		{
			int x_m = (px[0]+0x80)>>8;

			y1 = (nXBassFlt_B0 * x_m + nXBassFlt_B1 * x1 + nXBassFlt_A1 * y1) >> (10-8);
			x1 = x_m;
			px[0] += y1;
			y1 = (y1+0x40) >> 8;
			px++;
		}
		nXBassFlt_X1 = x1;
		nXBassFlt_Y1 = y1;
	}
	// Noise Reduction
	if (gdwSoundSetup & SNDMIX_NOISEREDUCTION)
	{
		int n = nLeftNR;
		int *pnr = MixSoundBuffer;
		for (int nr=count; nr; pnr++, nr--)
		{
			int vnr = *pnr >> 1;
			*pnr = vnr + n;
			n = vnr;
		}
		nLeftNR = n;
	}
}


#ifndef FASTSOUNDLIB

//////////////////////////////////////////////////////////////////////////
//
// DC Removal
//

#define DCR_AMOUNT		9

VOID MPPASMCALL X86_StereoDCRemoval(int *pBuffer, UINT nSamples)
{
	int y1l=nDCRFlt_Y1l, x1l=nDCRFlt_X1l;
	int y1r=nDCRFlt_Y1r, x1r=nDCRFlt_X1r;

	_asm {
	mov esi, pBuffer
	mov ecx, nSamples
stereodcr:
	mov eax, [esi]
	mov ebx, x1l
	mov edx, [esi+4]
	mov edi, x1r
	add esi, 8
	sub ebx, eax
	mov x1l, eax
	mov eax, ebx
	sar eax, DCR_AMOUNT+1
	sub edi, edx
	sub eax, ebx
	mov x1r, edx
	add eax, y1l
	mov edx, edi
	sar edx, DCR_AMOUNT+1
	mov [esi-8], eax
	sub edx, edi
	mov ebx, eax
	add edx, y1r
	sar ebx, DCR_AMOUNT
	mov [esi-4], edx
	mov edi, edx
	sub eax, ebx
	sar edi, DCR_AMOUNT
	mov y1l, eax
	sub edx, edi
	dec ecx
	mov y1r, edx
	jnz stereodcr
	}
	nDCRFlt_Y1l = y1l;
	nDCRFlt_X1l = x1l;
	nDCRFlt_Y1r = y1r;
	nDCRFlt_X1r = x1r;
}


VOID MPPASMCALL X86_MonoDCRemoval(int *pBuffer, UINT nSamples)
{
	_asm {
	mov esi, pBuffer
	mov ecx, nSamples
	mov edx, nDCRFlt_X1l
	mov edi, nDCRFlt_Y1l
stereodcr:
	mov eax, [esi]
	mov ebx, edx
	add esi, 4
	sub ebx, eax
	mov edx, eax
	mov eax, ebx
	sar eax, DCR_AMOUNT+1
	sub eax, ebx
	add eax, edi
	mov [esi-4], eax
	mov ebx, eax
	sar ebx, DCR_AMOUNT
	sub eax, ebx
	dec ecx
	mov edi, eax
	jnz stereodcr
	mov nDCRFlt_X1l, edx
	mov nDCRFlt_Y1l, edi
	}
}

#endif // FASTSOUNDLIB





/////////////////////////////////////////////////////////////////
// Clean DSP Effects interface

// [Reverb level 0(quiet)-100(loud)], [type = REVERBTYPE_XXXX]
BOOL CSoundFile::SetReverbParameters(UINT nDepth, UINT nType)
//-----------------------------------------------------------
{
	if (nDepth > 100) nDepth = 100;
	UINT gain = (nDepth * 16) / 100;
	if (gain > 16) gain = 16;
	if (gain < 1) gain = 1;
	m_nReverbDepth = gain;
	if (nType < NUM_REVERBTYPES) gnReverbType = nType;
	return TRUE;
}


// [XBass level 0(quiet)-100(loud)], [cutoff in Hz 20-100]
BOOL CSoundFile::SetXBassParameters(UINT nDepth, UINT nRange)
//-----------------------------------------------------------
{
	if (nDepth > 100) nDepth = 100;
	UINT gain = nDepth / 20;
	if (gain > 4) gain = 4;
	m_nXBassDepth = 8 - gain;	// filter attenuation 1/256 .. 1/16
	UINT range = nRange / 5;
	if (range > 5) range -= 5; else range = 0;
	if (nRange > 16) nRange = 16;
	m_nXBassRange = 21 - range;	// filter average on 0.5-1.6ms
	return TRUE;
}


// [Surround level 0(quiet)-100(heavy)] [delay in ms, usually 5-50ms]
BOOL CSoundFile::SetSurroundParameters(UINT nDepth, UINT nDelay)
//--------------------------------------------------------------
{
	UINT gain = (nDepth * 16) / 100;
	if (gain > 16) gain = 16;
	if (gain < 1) gain = 1;
	m_nProLogicDepth = gain;
	if (nDelay < 4) nDelay = 4;
	if (nDelay > 50) nDelay = 50;
	m_nProLogicDelay = nDelay;
	return TRUE;
}



