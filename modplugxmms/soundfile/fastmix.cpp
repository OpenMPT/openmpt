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

#ifdef WIN32
#pragma bss_seg(".modplug")
#endif

// Front Mix Buffer (Also room for interleaved rear mix)
int MixSoundBuffer[MIXBUFFERSIZE*4];

// Reverb Mix Buffer
#ifndef NO_REVERB
int MixReverbBuffer[MIXBUFFERSIZE*2];
extern UINT gnReverbSend;
#endif

#ifndef FASTSOUNDLIB
int MixRearBuffer[MIXBUFFERSIZE*2];
float MixFloatBuffer[MIXBUFFERSIZE*2];
#endif

#ifdef WIN32
#pragma bss_seg()
#endif


extern LONG gnDryROfsVol;
extern LONG gnDryLOfsVol;
extern LONG gnRvbROfsVol;
extern LONG gnRvbLOfsVol;

// 4x256 taps polyphase FIR resampling filter
extern short int gFastSinc[];
extern short int gKaiserSinc[]; // 8-taps polyphase

/////////////////////////////////////////////////////
// Mixing Macros

#define SNDMIX_BEGINSAMPLELOOP8\
	register MODCHANNEL * const pChn = pChannel;\
	nPos = pChn->nPosLo;\
	const signed char *p = (signed char *)(pChn->pCurrentSample+pChn->nPos);\
	if (pChn->dwFlags & CHN_STEREO) p += pChn->nPos;\
	int *pvol = pbuffer;\
	do {

#define SNDMIX_BEGINSAMPLELOOP16\
	register MODCHANNEL * const pChn = pChannel;\
	nPos = pChn->nPosLo;\
	const signed short *p = (signed short *)(pChn->pCurrentSample+(pChn->nPos*2));\
	if (pChn->dwFlags & CHN_STEREO) p += pChn->nPos;\
	int *pvol = pbuffer;\
	do {

#define SNDMIX_ENDSAMPLELOOP\
		nPos += pChn->nInc;\
	} while (pvol < pbufmax);\
	pChn->nPos += nPos >> 16;\
	pChn->nPosLo = nPos & 0xFFFF;

#define SNDMIX_ENDSAMPLELOOP8	SNDMIX_ENDSAMPLELOOP
#define SNDMIX_ENDSAMPLELOOP16	SNDMIX_ENDSAMPLELOOP

//////////////////////////////////////////////////////////////////////////////
// Mono

// No interpolation
#define SNDMIX_GETMONOVOL8NOIDO\
	int vol = p[nPos >> 16] << 8;

#define SNDMIX_GETMONOVOL16NOIDO\
	int vol = p[nPos >> 16];

// Linear Interpolation
#define SNDMIX_GETMONOVOL8LINEAR\
	int poshi = nPos >> 16;\
	int poslo = (nPos >> 8) & 0xFF;\
	int srcvol = p[poshi];\
	int destvol = p[poshi+1];\
	int vol = (srcvol<<8) + ((int)(poslo * (destvol - srcvol)));

#define SNDMIX_GETMONOVOL16LINEAR\
	int poshi = nPos >> 16;\
	int poslo = (nPos >> 8) & 0xFF;\
	int srcvol = p[poshi];\
	int destvol = p[poshi+1];\
	int vol = srcvol + ((int)(poslo * (destvol - srcvol)) >> 8);


/////////////////////////////////////////////////////////////////////////////
// Stereo

// No interpolation
#define SNDMIX_GETSTEREOVOL8NOIDO\
	int vol_l = p[(nPos>>16)*2] << 8;\
	int vol_r = p[(nPos>>16)*2+1] << 8;

#define SNDMIX_GETSTEREOVOL16NOIDO\
	int vol_l = p[(nPos>>16)*2];\
	int vol_r = p[(nPos>>16)*2+1];

// Linear Interpolation
#define SNDMIX_GETSTEREOVOL8LINEAR\
	int poshi = nPos >> 16;\
	int poslo = (nPos >> 8) & 0xFF;\
	int srcvol_l = p[poshi*2];\
	int vol_l = (srcvol_l<<8) + ((int)(poslo * (p[poshi*2+2] - srcvol_l)));\
	int srcvol_r = p[poshi*2+1];\
	int vol_r = (srcvol_r<<8) + ((int)(poslo * (p[poshi*2+3] - srcvol_r)));

#define SNDMIX_GETSTEREOVOL16LINEAR\
	int poshi = nPos >> 16;\
	int poslo = (nPos >> 8) & 0xFF;\
	int srcvol_l = p[poshi*2];\
	int vol_l = srcvol_l + ((int)(poslo * (p[poshi*2+2] - srcvol_l)) >> 8);\
	int srcvol_r = p[poshi*2+1];\
	int vol_r = srcvol_r + ((int)(poslo * (p[poshi*2+3] - srcvol_r)) >> 8);\


/////////////////////////////////////////////////////////////////////////////

#define SNDMIX_STOREMONOVOL\
	pvol[0] += vol * pChn->nRightVol;\
	pvol[1] += vol * pChn->nLeftVol;\
	pvol += 2;

#define SNDMIX_STORESTEREOVOL\
	pvol[0] += vol_l * pChn->nRightVol;\
	pvol[1] += vol_r * pChn->nLeftVol;\
	pvol += 2;

#define SNDMIX_STOREFASTMONOVOL\
	int v = vol * pChn->nRightVol;\
	pvol[0] += v;\
	pvol[1] += v;\
	pvol += 2;

#define SNDMIX_RAMPMONOVOL\
	nRampLeftVol += pChn->nLeftRamp;\
	nRampRightVol += pChn->nRightRamp;\
	pvol[0] += vol * (nRampRightVol >> VOLUMERAMPPRECISION);\
	pvol[1] += vol * (nRampLeftVol >> VOLUMERAMPPRECISION);\
	pvol += 2;

#define SNDMIX_RAMPFASTMONOVOL\
	nRampRightVol += pChn->nRightRamp;\
	int fastvol = vol * (nRampRightVol >> VOLUMERAMPPRECISION);\
	pvol[0] += fastvol;\
	pvol[1] += fastvol;\
	pvol += 2;

#define SNDMIX_RAMPSTEREOVOL\
	nRampLeftVol += pChn->nLeftRamp;\
	nRampRightVol += pChn->nRightRamp;\
	pvol[0] += vol_l * (nRampRightVol >> VOLUMERAMPPRECISION);\
	pvol[1] += vol_r * (nRampLeftVol >> VOLUMERAMPPRECISION);\
	pvol += 2;


///////////////////////////////////////////////////
// Resonant Filters

// Mono
#define MIX_BEGIN_FILTER\
	int fy1 = pChannel->nFilter_Y1;\
	int fy2 = pChannel->nFilter_Y2;\

#define MIX_END_FILTER\
	pChannel->nFilter_Y1 = fy1;\
	pChannel->nFilter_Y2 = fy2;

#define SNDMIX_PROCESSFILTER\
	vol = (vol * pChn->nFilter_A0 + fy1 * pChn->nFilter_B0 + fy2 * pChn->nFilter_B1 + 4096) >> 13;\
	fy2 = fy1;\
	fy1 = vol;\

// Stereo
#define MIX_BEGIN_STEREO_FILTER\
	int fy1 = pChannel->nFilter_Y1;\
	int fy2 = pChannel->nFilter_Y2;\
	int fy3 = pChannel->nFilter_Y3;\
	int fy4 = pChannel->nFilter_Y4;\

#define MIX_END_STEREO_FILTER\
	pChannel->nFilter_Y1 = fy1;\
	pChannel->nFilter_Y2 = fy2;\
	pChannel->nFilter_Y3 = fy3;\
	pChannel->nFilter_Y4 = fy4;\

#define SNDMIX_PROCESSSTEREOFILTER\
	vol_l = (vol_l * pChn->nFilter_A0 + fy1 * pChn->nFilter_B0 + fy2 * pChn->nFilter_B1 + 4096) >> 13;\
	vol_r = (vol_r * pChn->nFilter_A0 + fy3 * pChn->nFilter_B0 + fy4 * pChn->nFilter_B1 + 4096) >> 13;\
	fy2 = fy1; fy1 = vol_l;\
	fy4 = fy3; fy3 = vol_r;\

//////////////////////////////////////////////////////////
// Interfaces

typedef VOID (MPPASMCALL * LPMIXINTERFACE)(MODCHANNEL *, int *, int *);

#define BEGIN_MIX_INTERFACE(func)\
	VOID MPPASMCALL func(MODCHANNEL *pChannel, int *pbuffer, int *pbufmax)\
	{\
		LONG nPos;

#define END_MIX_INTERFACE()\
		SNDMIX_ENDSAMPLELOOP\
	}

// Volume Ramps
#define BEGIN_RAMPMIX_INTERFACE(func)\
	BEGIN_MIX_INTERFACE(func)\
		LONG nRampRightVol = pChannel->nRampRightVol;\
		LONG nRampLeftVol = pChannel->nRampLeftVol;

#define END_RAMPMIX_INTERFACE()\
		SNDMIX_ENDSAMPLELOOP\
		pChannel->nRampRightVol = nRampRightVol;\
		pChannel->nRightVol = nRampRightVol >> VOLUMERAMPPRECISION;\
		pChannel->nRampLeftVol = nRampLeftVol;\
		pChannel->nLeftVol = nRampLeftVol >> VOLUMERAMPPRECISION;\
	}

#define BEGIN_FASTRAMPMIX_INTERFACE(func)\
	BEGIN_MIX_INTERFACE(func)\
		LONG nRampRightVol = pChannel->nRampRightVol;

#define END_FASTRAMPMIX_INTERFACE()\
		SNDMIX_ENDSAMPLELOOP\
		pChannel->nRampRightVol = nRampRightVol;\
		pChannel->nRampLeftVol = nRampRightVol;\
		pChannel->nRightVol = nRampRightVol >> VOLUMERAMPPRECISION;\
		pChannel->nLeftVol = pChannel->nRightVol;\
	}


// Mono Resonant Filters
#define BEGIN_MIX_FLT_INTERFACE(func)\
	BEGIN_MIX_INTERFACE(func)\
	MIX_BEGIN_FILTER
	

#define END_MIX_FLT_INTERFACE()\
	SNDMIX_ENDSAMPLELOOP\
	MIX_END_FILTER\
	}

#define BEGIN_RAMPMIX_FLT_INTERFACE(func)\
	BEGIN_MIX_INTERFACE(func)\
		LONG nRampRightVol = pChannel->nRampRightVol;\
		LONG nRampLeftVol = pChannel->nRampLeftVol;\
		MIX_BEGIN_FILTER

#define END_RAMPMIX_FLT_INTERFACE()\
		SNDMIX_ENDSAMPLELOOP\
		MIX_END_FILTER\
		pChannel->nRampRightVol = nRampRightVol;\
		pChannel->nRightVol = nRampRightVol >> VOLUMERAMPPRECISION;\
		pChannel->nRampLeftVol = nRampLeftVol;\
		pChannel->nLeftVol = nRampLeftVol >> VOLUMERAMPPRECISION;\
	}

// Stereo Resonant Filters
#define BEGIN_MIX_STFLT_INTERFACE(func)\
	BEGIN_MIX_INTERFACE(func)\
	MIX_BEGIN_STEREO_FILTER
	

#define END_MIX_STFLT_INTERFACE()\
	SNDMIX_ENDSAMPLELOOP\
	MIX_END_STEREO_FILTER\
	}

#define BEGIN_RAMPMIX_STFLT_INTERFACE(func)\
	BEGIN_MIX_INTERFACE(func)\
		LONG nRampRightVol = pChannel->nRampRightVol;\
		LONG nRampLeftVol = pChannel->nRampLeftVol;\
		MIX_BEGIN_STEREO_FILTER

#define END_RAMPMIX_STFLT_INTERFACE()\
		SNDMIX_ENDSAMPLELOOP\
		MIX_END_STEREO_FILTER\
		pChannel->nRampRightVol = nRampRightVol;\
		pChannel->nRightVol = nRampRightVol >> VOLUMERAMPPRECISION;\
		pChannel->nRampLeftVol = nRampLeftVol;\
		pChannel->nLeftVol = nRampLeftVol >> VOLUMERAMPPRECISION;\
	}


/////////////////////////////////////////////////////
//

void MPPASMCALL X86_InitMixBuffer(int *pBuffer, UINT nSamples);
void MPPASMCALL X86_EndChannelOfs(MODCHANNEL *pChannel, int *pBuffer, UINT nSamples);
void MPPASMCALL X86_StereoFill(int *pBuffer, UINT nSamples, LPLONG lpROfs, LPLONG lpLOfs);
void X86_StereoMixToFloat(const int *, float *, float *, UINT nCount);
void X86_FloatToStereoMix(const float *pIn1, const float *pIn2, int *pOut, UINT nCount);

/////////////////////////////////////////////////////
// Mono samples functions

BEGIN_MIX_INTERFACE(Mono8BitMix)
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETMONOVOL8NOIDO
	SNDMIX_STOREMONOVOL
END_MIX_INTERFACE()

BEGIN_MIX_INTERFACE(Mono16BitMix)
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETMONOVOL16NOIDO
	SNDMIX_STOREMONOVOL
END_MIX_INTERFACE()

BEGIN_MIX_INTERFACE(Mono8BitLinearMix)
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETMONOVOL8LINEAR
	SNDMIX_STOREMONOVOL
END_MIX_INTERFACE()

BEGIN_MIX_INTERFACE(Mono16BitLinearMix)
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETMONOVOL16LINEAR
	SNDMIX_STOREMONOVOL
END_MIX_INTERFACE()


// Volume Ramps
BEGIN_RAMPMIX_INTERFACE(Mono8BitRampMix)
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETMONOVOL8NOIDO
	SNDMIX_RAMPMONOVOL
END_RAMPMIX_INTERFACE()

BEGIN_RAMPMIX_INTERFACE(Mono16BitRampMix)
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETMONOVOL16NOIDO
	SNDMIX_RAMPMONOVOL
END_RAMPMIX_INTERFACE()

BEGIN_RAMPMIX_INTERFACE(Mono8BitLinearRampMix)
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETMONOVOL8LINEAR
	SNDMIX_RAMPMONOVOL
END_RAMPMIX_INTERFACE()

BEGIN_RAMPMIX_INTERFACE(Mono16BitLinearRampMix)
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETMONOVOL16LINEAR
	SNDMIX_RAMPMONOVOL
END_RAMPMIX_INTERFACE()

//////////////////////////////////////////////////////
// Fast mono mix for leftvol=rightvol (1 less imul)

BEGIN_MIX_INTERFACE(FastMono8BitMix)
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETMONOVOL8NOIDO
	SNDMIX_STOREFASTMONOVOL
END_MIX_INTERFACE()

BEGIN_MIX_INTERFACE(FastMono16BitMix)
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETMONOVOL16NOIDO
	SNDMIX_STOREFASTMONOVOL
END_MIX_INTERFACE()

BEGIN_MIX_INTERFACE(FastMono8BitLinearMix)
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETMONOVOL8LINEAR
	SNDMIX_STOREFASTMONOVOL
END_MIX_INTERFACE()

BEGIN_MIX_INTERFACE(FastMono16BitLinearMix)
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETMONOVOL16LINEAR
	SNDMIX_STOREFASTMONOVOL
END_MIX_INTERFACE()

// Fast Ramps
BEGIN_FASTRAMPMIX_INTERFACE(FastMono8BitRampMix)
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETMONOVOL8NOIDO
	SNDMIX_RAMPFASTMONOVOL
END_FASTRAMPMIX_INTERFACE()

BEGIN_FASTRAMPMIX_INTERFACE(FastMono16BitRampMix)
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETMONOVOL16NOIDO
	SNDMIX_RAMPFASTMONOVOL
END_FASTRAMPMIX_INTERFACE()

BEGIN_FASTRAMPMIX_INTERFACE(FastMono8BitLinearRampMix)
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETMONOVOL8LINEAR
	SNDMIX_RAMPFASTMONOVOL
END_FASTRAMPMIX_INTERFACE()

BEGIN_FASTRAMPMIX_INTERFACE(FastMono16BitLinearRampMix)
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETMONOVOL16LINEAR
	SNDMIX_RAMPFASTMONOVOL
END_FASTRAMPMIX_INTERFACE()


//////////////////////////////////////////////////////
// Stereo samples

BEGIN_MIX_INTERFACE(Stereo8BitMix)
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETSTEREOVOL8NOIDO
	SNDMIX_STORESTEREOVOL
END_MIX_INTERFACE()

BEGIN_MIX_INTERFACE(Stereo16BitMix)
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETSTEREOVOL16NOIDO
	SNDMIX_STORESTEREOVOL
END_MIX_INTERFACE()

BEGIN_MIX_INTERFACE(Stereo8BitLinearMix)
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETSTEREOVOL8LINEAR
	SNDMIX_STORESTEREOVOL
END_MIX_INTERFACE()

BEGIN_MIX_INTERFACE(Stereo16BitLinearMix)
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETSTEREOVOL16LINEAR
	SNDMIX_STORESTEREOVOL
END_MIX_INTERFACE()

// Volume Ramps
BEGIN_RAMPMIX_INTERFACE(Stereo8BitRampMix)
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETSTEREOVOL8NOIDO
	SNDMIX_RAMPSTEREOVOL
END_RAMPMIX_INTERFACE()

BEGIN_RAMPMIX_INTERFACE(Stereo16BitRampMix)
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETSTEREOVOL16NOIDO
	SNDMIX_RAMPSTEREOVOL
END_RAMPMIX_INTERFACE()

BEGIN_RAMPMIX_INTERFACE(Stereo8BitLinearRampMix)
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETSTEREOVOL8LINEAR
	SNDMIX_RAMPSTEREOVOL
END_RAMPMIX_INTERFACE()

BEGIN_RAMPMIX_INTERFACE(Stereo16BitLinearRampMix)
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETSTEREOVOL16LINEAR
	SNDMIX_RAMPSTEREOVOL
END_RAMPMIX_INTERFACE()


//////////////////////////////////////////////////////
// Resonant Filter Mix

#ifndef NO_FILTER

// Mono Filter Mix
BEGIN_MIX_FLT_INTERFACE(FilterMono8BitMix)
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETMONOVOL8NOIDO
	SNDMIX_PROCESSFILTER
	SNDMIX_STOREMONOVOL
END_MIX_FLT_INTERFACE()

BEGIN_MIX_FLT_INTERFACE(FilterMono16BitMix)
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETMONOVOL16NOIDO
	SNDMIX_PROCESSFILTER
	SNDMIX_STOREMONOVOL
END_MIX_FLT_INTERFACE()

BEGIN_MIX_FLT_INTERFACE(FilterMono8BitLinearMix)
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETMONOVOL8LINEAR
	SNDMIX_PROCESSFILTER
	SNDMIX_STOREMONOVOL
END_MIX_FLT_INTERFACE()

BEGIN_MIX_FLT_INTERFACE(FilterMono16BitLinearMix)
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETMONOVOL16LINEAR
	SNDMIX_PROCESSFILTER
	SNDMIX_STOREMONOVOL
END_MIX_FLT_INTERFACE()

// Filter + Ramp
BEGIN_RAMPMIX_FLT_INTERFACE(FilterMono8BitRampMix)
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETMONOVOL8NOIDO
	SNDMIX_PROCESSFILTER
	SNDMIX_RAMPMONOVOL
END_RAMPMIX_FLT_INTERFACE()

BEGIN_RAMPMIX_FLT_INTERFACE(FilterMono16BitRampMix)
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETMONOVOL16NOIDO
	SNDMIX_PROCESSFILTER
	SNDMIX_RAMPMONOVOL
END_RAMPMIX_FLT_INTERFACE()

BEGIN_RAMPMIX_FLT_INTERFACE(FilterMono8BitLinearRampMix)
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETMONOVOL8LINEAR
	SNDMIX_PROCESSFILTER
	SNDMIX_RAMPMONOVOL
END_RAMPMIX_FLT_INTERFACE()

BEGIN_RAMPMIX_FLT_INTERFACE(FilterMono16BitLinearRampMix)
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETMONOVOL16LINEAR
	SNDMIX_PROCESSFILTER
	SNDMIX_RAMPMONOVOL
END_RAMPMIX_FLT_INTERFACE()


// Stereo Filter Mix
BEGIN_MIX_STFLT_INTERFACE(FilterStereo8BitMix)
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETSTEREOVOL8NOIDO
	SNDMIX_PROCESSSTEREOFILTER
	SNDMIX_STORESTEREOVOL
END_MIX_STFLT_INTERFACE()

BEGIN_MIX_STFLT_INTERFACE(FilterStereo16BitMix)
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETSTEREOVOL16NOIDO
	SNDMIX_PROCESSSTEREOFILTER
	SNDMIX_STORESTEREOVOL
END_MIX_STFLT_INTERFACE()

BEGIN_MIX_STFLT_INTERFACE(FilterStereo8BitLinearMix)
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETSTEREOVOL8LINEAR
	SNDMIX_PROCESSSTEREOFILTER
	SNDMIX_STORESTEREOVOL
END_MIX_STFLT_INTERFACE()

BEGIN_MIX_STFLT_INTERFACE(FilterStereo16BitLinearMix)
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETSTEREOVOL16LINEAR
	SNDMIX_PROCESSSTEREOFILTER
	SNDMIX_STORESTEREOVOL
END_MIX_STFLT_INTERFACE()

// Stereo Filter + Ramp
BEGIN_RAMPMIX_STFLT_INTERFACE(FilterStereo8BitRampMix)
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETSTEREOVOL8NOIDO
	SNDMIX_PROCESSSTEREOFILTER
	SNDMIX_RAMPSTEREOVOL
END_RAMPMIX_STFLT_INTERFACE()

BEGIN_RAMPMIX_STFLT_INTERFACE(FilterStereo16BitRampMix)
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETSTEREOVOL16NOIDO
	SNDMIX_PROCESSSTEREOFILTER
	SNDMIX_RAMPSTEREOVOL
END_RAMPMIX_STFLT_INTERFACE()

BEGIN_RAMPMIX_STFLT_INTERFACE(FilterStereo8BitLinearRampMix)
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETSTEREOVOL8LINEAR
	SNDMIX_PROCESSSTEREOFILTER
	SNDMIX_RAMPSTEREOVOL
END_RAMPMIX_STFLT_INTERFACE()

BEGIN_RAMPMIX_STFLT_INTERFACE(FilterStereo16BitLinearRampMix)
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETSTEREOVOL16LINEAR
	SNDMIX_PROCESSSTEREOFILTER
	SNDMIX_RAMPSTEREOVOL
END_RAMPMIX_STFLT_INTERFACE()

#else
// Mono
#define FilterMono8BitMix				Mono8BitMix
#define FilterMono16BitMix				Mono16BitMix
#define FilterMono8BitLinearMix			Mono8BitLinearMix
#define FilterMono16BitLinearMix		Mono16BitLinearMix
#define FilterMono8BitRampMix			Mono8BitRampMix
#define FilterMono16BitRampMix			Mono16BitRampMix
#define FilterMono8BitLinearRampMix		Mono8BitLinearRampMix
#define FilterMono16BitLinearRampMix	Mono16BitLinearRampMix
// Stereo
#define FilterStereo8BitMix				Stereo8BitMix
#define FilterStereo16BitMix			Stereo16BitMix
#define FilterStereo8BitLinearMix		Stereo8BitLinearMix
#define FilterStereo16BitLinearMix		Stereo16BitLinearMix
#define FilterStereo8BitRampMix			Stereo8BitRampMix
#define FilterStereo16BitRampMix		Stereo16BitRampMix
#define FilterStereo8BitLinearRampMix	Stereo8BitLinearRampMix
#define FilterStereo16BitLinearRampMix	Stereo16BitLinearRampMix
#endif

/////////////////////////////////////////////////////////////////////////////////////
//
// Mix function tables
//
//
// Index is as follow:
//	[b1-b0]	format (8-bit-mono, 16-bit-mono, 8-bit-stereo, 16-bit-stereo)
//	[b2]	ramp
//	[b3]	filter
//	[b5-b4]	src type
//

#define MIXNDX_16BIT	0x01
#define MIXNDX_STEREO	0x02
#define MIXNDX_RAMP		0x04
#define MIXNDX_FILTER	0x08
#define MIXNDX_LINEARSRC	0x10


const LPMIXINTERFACE gpMixFunctionTable[2*16] =
{
	// No SRC
	Mono8BitMix,				Mono16BitMix,				Stereo8BitMix,			Stereo16BitMix,
	Mono8BitRampMix,			Mono16BitRampMix,			Stereo8BitRampMix,		Stereo16BitRampMix,
	// No SRC, Filter
	FilterMono8BitMix,			FilterMono16BitMix,			FilterStereo8BitMix,	FilterStereo16BitMix,
	FilterMono8BitRampMix,		FilterMono16BitRampMix,		FilterStereo8BitRampMix,FilterStereo16BitRampMix,
	// Linear SRC
	Mono8BitLinearMix,			Mono16BitLinearMix,			Stereo8BitLinearMix,	Stereo16BitLinearMix,
	Mono8BitLinearRampMix,		Mono16BitLinearRampMix,		Stereo8BitLinearRampMix,Stereo16BitLinearRampMix,
	// Linear SRC, Filter
	FilterMono8BitLinearMix,	FilterMono16BitLinearMix,	FilterStereo8BitLinearMix,	FilterStereo16BitLinearMix,
	FilterMono8BitLinearRampMix,FilterMono16BitLinearRampMix,FilterStereo8BitLinearRampMix,FilterStereo16BitLinearRampMix,
};

const LPMIXINTERFACE gpFastMixFunctionTable[2*16] =
{
	// No SRC
	FastMono8BitMix,			FastMono16BitMix,			Stereo8BitMix,			Stereo16BitMix,
	FastMono8BitRampMix,		FastMono16BitRampMix,		Stereo8BitRampMix,		Stereo16BitRampMix,
	// No SRC, Filter
	FilterMono8BitMix,			FilterMono16BitMix,			FilterStereo8BitMix,	FilterStereo16BitMix,
	FilterMono8BitRampMix,		FilterMono16BitRampMix,		FilterStereo8BitRampMix,FilterStereo16BitRampMix,
	// Linear SRC
	FastMono8BitLinearMix,		FastMono16BitLinearMix,		Stereo8BitLinearMix,	Stereo16BitLinearMix,
	FastMono8BitLinearRampMix,	FastMono16BitLinearRampMix,	Stereo8BitLinearRampMix,Stereo16BitLinearRampMix,
	// Linear SRC, Filter
	FilterMono8BitLinearMix,	FilterMono16BitLinearMix,	FilterStereo8BitLinearMix,	FilterStereo16BitLinearMix,
	FilterMono8BitLinearRampMix,FilterMono16BitLinearRampMix,FilterStereo8BitLinearRampMix,FilterStereo16BitLinearRampMix,
};


/////////////////////////////////////////////////////////////////////////

static LONG MPPFASTCALL GetSampleCount(MODCHANNEL *pChn, LONG nSamples)
//---------------------------------------------------------------------
{
	LONG nLoopStart = (pChn->dwFlags & CHN_LOOP) ? pChn->nLoopStart : 0;
	LONG nInc = pChn->nInc;

	if ((nSamples <= 0) || (!nInc) || (!pChn->nLength)) return 0;
	// Under zero ?
	if ((LONG)pChn->nPos < nLoopStart)
	{
		if (nInc < 0)
		{
			// Invert loop for bidi loops
			LONG nDelta = ((nLoopStart - pChn->nPos) << 16) - (pChn->nPosLo & 0xffff);
			pChn->nPos = nLoopStart | (nDelta>>16);
			pChn->nPosLo = nDelta & 0xffff;
			if (((LONG)pChn->nPos < nLoopStart) || (pChn->nPos >= (nLoopStart+pChn->nLength)/2))
			{
				pChn->nPos = nLoopStart; pChn->nPosLo = 0;
			}
			nInc = -nInc;
			pChn->nInc = nInc;
			pChn->dwFlags &= ~(CHN_PINGPONGFLAG); // go forward
			if ((!(pChn->dwFlags & CHN_LOOP)) || (pChn->nPos >= pChn->nLength))
			{
				pChn->nPos = pChn->nLength;
				pChn->nPosLo = 0;
				return 0;
			}
		} else
		{
			// We probably didn't hit the loop end yet (first loop), so we do nothing
			if ((LONG)pChn->nPos < 0) pChn->nPos = 0;
		}
	} else
	// Past the end
	if (pChn->nPos >= pChn->nLength)
	{
		if (!(pChn->dwFlags & CHN_LOOP)) return 0; // not looping -> stop this channel
		if (pChn->dwFlags & CHN_PINGPONGLOOP)
		{
			// Invert loop
			if (nInc > 0)
			{
				nInc = -nInc;
				pChn->nInc = nInc;
			}
			pChn->dwFlags |= CHN_PINGPONGFLAG;
			// adjust loop position
			LONG nDeltaHi = (pChn->nPos - pChn->nLength);
			LONG nDeltaLo = 0x10000 - (pChn->nPosLo & 0xffff);
			pChn->nPos = pChn->nLength - nDeltaHi - (nDeltaLo>>16);
			pChn->nPosLo = nDeltaLo & 0xffff;
			if ((pChn->nPos <= pChn->nLoopStart) || (pChn->nPos >= pChn->nLength)) pChn->nPos = pChn->nLength-1;
		} else
		{
			if (nInc < 0) // This is a bug
			{
				nInc = -nInc;
				pChn->nInc = nInc;
			}
			// Restart at loop start
			pChn->nPos += nLoopStart - pChn->nLength;
			if ((LONG)pChn->nPos < nLoopStart) pChn->nPos = pChn->nLoopStart;
		}
	}
	LONG nPos = pChn->nPos;
	// too big increment, and/or too small loop length
	if (nPos < nLoopStart)
	{
		if ((nPos < 0) || (nInc < 0)) return 0;
	}
	if ((nPos < 0) || (nPos >= (LONG)pChn->nLength)) return 0;
	LONG nPosLo = (USHORT)pChn->nPosLo, nSmpCount = nSamples;
	if (nInc < 0)
	{
		LONG nInv = -nInc;
		LONG maxsamples = 16384 / ((nInv>>16)+1);
		if (maxsamples < 2) maxsamples = 2;
		if (nSamples > maxsamples) nSamples = maxsamples;
		LONG nDeltaHi = (nInv>>16) * (nSamples - 1);
		LONG nDeltaLo = (nInv&0xffff) * (nSamples - 1);
		LONG nPosDest = nPos - nDeltaHi + ((nPosLo - nDeltaLo) >> 16);
		if (nPosDest < nLoopStart)
		{
			nSmpCount = (ULONG)(((((LONGLONG)nPos - nLoopStart) << 16) + nPosLo - 1) / nInv) + 1;
		}
	} else
	{
		LONG maxsamples = 16384 / ((nInc>>16)+1);
		if (maxsamples < 2) maxsamples = 2;
		if (nSamples > maxsamples) nSamples = maxsamples;
		LONG nDeltaHi = (nInc>>16) * (nSamples - 1);
		LONG nDeltaLo = (nInc&0xffff) * (nSamples - 1);
		LONG nPosDest = nPos + nDeltaHi + ((nPosLo + nDeltaLo)>>16);
		if (nPosDest >= (LONG)pChn->nLength)
		{
			nSmpCount = (ULONG)(((((LONGLONG)pChn->nLength - nPos) << 16) - nPosLo - 1) / nInc) + 1;
		}
	}
	if (nSmpCount <= 1) return 1;
	if (nSmpCount > nSamples) return nSamples;
	return nSmpCount;
}


UINT CSoundFile::CreateStereoMix(int count)
//-----------------------------------------
{
	LPLONG pOfsL, pOfsR;
	DWORD nchused, nchmixed;
	
	if (!count) return 0;
#ifndef FASTSOUNDLIB
	if (gnChannels > 2) X86_InitMixBuffer(MixRearBuffer, count*2);
#endif
	nchused = nchmixed = 0;
	for (UINT nChn=0; nChn<m_nMixChannels; nChn++)
	{
		const LPMIXINTERFACE *pMixFuncTable;
		MODCHANNEL * const pChannel = &Chn[ChnMix[nChn]];
		UINT nFlags, nMasterCh;
		LONG nSmpCount;
		int nsamples;
		int *pbuffer;

		if (!pChannel->pCurrentSample) continue;
		nMasterCh = (ChnMix[nChn] < m_nChannels) ? ChnMix[nChn]+1 : pChannel->nMasterChn;
		pOfsR = &gnDryROfsVol;
		pOfsL = &gnDryLOfsVol;
		nFlags = 0;
		if (pChannel->dwFlags & CHN_16BIT) nFlags |= MIXNDX_16BIT;
		if (pChannel->dwFlags & CHN_STEREO) nFlags |= MIXNDX_STEREO;
	#ifndef NO_FILTER
		if (pChannel->dwFlags & CHN_FILTER) nFlags |= MIXNDX_FILTER;
	#endif
		if (!(pChannel->dwFlags & CHN_NOIDO))
		{
			nFlags += MIXNDX_LINEARSRC;
		}
		if ((nFlags < 0x20) && (pChannel->nLeftVol == pChannel->nRightVol)
		 && ((!pChannel->nRampLength) || (pChannel->nLeftRamp == pChannel->nRightRamp)))
		{
			pMixFuncTable = gpFastMixFunctionTable;
		} else
		{
			pMixFuncTable = gpMixFunctionTable;
		}
		nsamples = count;
#ifndef NO_REVERB
		pbuffer = (gdwSoundSetup & SNDMIX_REVERB) ? MixReverbBuffer : MixSoundBuffer;
		if (pChannel->dwFlags & CHN_NOREVERB) pbuffer = MixSoundBuffer;
		if (pChannel->dwFlags & CHN_REVERB) pbuffer = MixReverbBuffer;
		if (pbuffer == MixReverbBuffer)
		{
			if (!gnReverbSend) memset(MixReverbBuffer, 0, count * 8);
			gnReverbSend += count;
		}
#else
		pbuffer = MixSoundBuffer;
#endif
		nchused++;
		////////////////////////////////////////////////////
	SampleLooping:
		UINT nrampsamples = nsamples;
		if (pChannel->nRampLength > 0)
		{
			if ((LONG)nrampsamples > pChannel->nRampLength) nrampsamples = pChannel->nRampLength;
		}
		if ((nSmpCount = GetSampleCount(pChannel, nrampsamples)) <= 0)
		{
			// Stopping the channel
			pChannel->pCurrentSample = NULL;
			pChannel->nLength = 0;
			pChannel->nPos = 0;
			pChannel->nPosLo = 0;
			pChannel->nRampLength = 0;
			X86_EndChannelOfs(pChannel, pbuffer, nsamples);
			*pOfsR += pChannel->nROfs;
			*pOfsL += pChannel->nLOfs;
			pChannel->nROfs = pChannel->nLOfs = 0;
			pChannel->dwFlags &= ~CHN_PINGPONGFLAG;
			continue;
		}
		// Should we mix this channel ?
		UINT naddmix;
		if (((nchmixed >= m_nMaxMixChannels) && (!(gdwSoundSetup & SNDMIX_DIRECTTODISK)))
		 || ((!pChannel->nRampLength) && (!(pChannel->nLeftVol|pChannel->nRightVol))))
		{
			LONG delta = (pChannel->nInc * (LONG)nSmpCount) + (LONG)pChannel->nPosLo;
			pChannel->nPosLo = delta & 0xFFFF;
			pChannel->nPos += (delta >> 16);
			pChannel->nROfs = pChannel->nLOfs = 0;
			pbuffer += nSmpCount*2;
			naddmix = 0;
		} else
		// Do mixing
		{
			// Choose function for mixing
			LPMIXINTERFACE pMixFunc;
			pMixFunc = (pChannel->nRampLength) ? pMixFuncTable[nFlags|MIXNDX_RAMP] : pMixFuncTable[nFlags];
			int *pbufmax = pbuffer + (nSmpCount*2);
			pChannel->nROfs = - *(pbufmax-2);
			pChannel->nLOfs = - *(pbufmax-1);
			pMixFunc(pChannel, pbuffer, pbufmax);
			pChannel->nROfs += *(pbufmax-2);
			pChannel->nLOfs += *(pbufmax-1);
			pbuffer = pbufmax;
			naddmix = 1;
		}
		nsamples -= nSmpCount;
		if (pChannel->nRampLength)
		{
			pChannel->nRampLength -= nSmpCount;
			if (pChannel->nRampLength <= 0)
			{
				pChannel->nRampLength = 0;
				pChannel->nRightVol = pChannel->nNewRightVol;
				pChannel->nLeftVol = pChannel->nNewLeftVol;
				pChannel->nRightRamp = pChannel->nLeftRamp = 0;
				if ((pChannel->dwFlags & CHN_NOTEFADE) && (!(pChannel->nFadeOutVol)))
				{
					pChannel->nLength = 0;
					pChannel->pCurrentSample = NULL;
				}
			}
		}
		if (nsamples > 0) goto SampleLooping;
		nchmixed += naddmix;
	}
	return nchused;
}


#ifdef WIN32
#pragma warning (disable:4100)
#endif

// Clip and convert to 8 bit
#ifdef WIN32
__declspec(naked) DWORD MPPASMCALL X86_Convert32To8(LPVOID lp16, int *pBuffer, DWORD lSampleCount, LPLONG lpMin, LPLONG lpMax)
//----------------------------------------------------------------------------------------------------------------------------
{
	_asm {
	push ebx
	push esi
	push edi
	mov ebx, 16[esp]		// ebx = 8-bit buffer
	mov esi, 20[esp]		// esi = pBuffer
	mov edi, 24[esp]		// edi = lSampleCount
	mov eax, 28[esp]
	mov ecx, dword ptr [eax]	// ecx = clipmin
	mov eax, 32[esp]
	mov edx, dword ptr [eax]	// edx = clipmax
cliploop:
	mov eax, dword ptr [esi]
	inc ebx
	cdq
	and edx, (1 << (24-MIXING_ATTENUATION)) - 1
	add eax, edx
	cmp eax, MIXING_CLIPMIN
	jl cliplow
	cmp eax, MIXING_CLIPMAX
	jg cliphigh
	cmp eax, ecx
	jl updatemin
	cmp eax, edx
	jg updatemax
cliprecover:
	add esi, 4
	sar eax, 24-MIXING_ATTENUATION
	xor eax, 0x80
	dec edi
	mov byte ptr [ebx-1], al
	jnz cliploop
	mov eax, 28[esp]
	mov dword ptr [eax], ecx
	mov eax, 32[esp]
	mov dword ptr [eax], edx
	mov eax, 24[esp]
	pop edi
	pop esi
	pop ebx
	ret
updatemin:
	mov ecx, eax
	jmp cliprecover
updatemax:
	mov edx, eax
	jmp cliprecover
cliplow:
	mov ecx, MIXING_CLIPMIN
	mov edx, MIXING_CLIPMAX
	mov eax, MIXING_CLIPMIN
	jmp cliprecover
cliphigh:
	mov ecx, MIXING_CLIPMIN
	mov edx, MIXING_CLIPMAX
	mov eax, MIXING_CLIPMAX
	jmp cliprecover
	}
}
#else //WIN32
//---GCCFIX: Asm replaced with C function
// The C version was written by Rani Assaf <rani@magic.metawire.com>, I believe
__declspec(naked) DWORD MPPASMCALL X86_Convert32To8(LPVOID lp8, int *pBuffer, DWORD lSampleCount, LPLONG lpMin, LPLONG lpMax)
{
	int vumin = *lpMin, vumax = *lpMax;
	unsigned char *p = (unsigned char *)lp8;
	for (UINT i=0; i<lSampleCount; i++)
	{
		int n = pBuffer[i];
		if (n < MIXING_CLIPMIN)	
			n = MIXING_CLIPMIN;
		else if (n > MIXING_CLIPMAX)
			n = MIXING_CLIPMAX;
		if (n < vumin)
			vumin = n;
		else if (n > vumax)
			vumax = n;
		p[i] = (n >> (24-MIXING_ATTENUATION)) ^ 0x80;	// 8-bit unsigned
	}
	*lpMin = vumin;
	*lpMax = vumax;
	return lSampleCount;
}
#endif //WIN32, else


#ifdef WIN32
// Clip and convert to 16 bit
__declspec(naked) DWORD MPPASMCALL X86_Convert32To16(LPVOID lp16, int *pBuffer, DWORD lSampleCount, LPLONG lpMin, LPLONG lpMax)
//-----------------------------------------------------------------------------------------------------------------------------
{
	_asm {
	push ebx
	push esi
	push edi
	mov ebx, 16[esp]		// ebx = 16-bit buffer
	mov eax, 28[esp]
	mov esi, 20[esp]		// esi = pBuffer
	mov ecx, dword ptr [eax]	// ecx = clipmin
	mov edi, 24[esp]		// edi = lSampleCount
	mov eax, 32[esp]
	push ebp
	mov ebp, dword ptr [eax]	// edx = clipmax
cliploop:
	mov eax, dword ptr [esi]
	add ebx, 2
	cdq
	and edx, (1 << (16-MIXING_ATTENUATION)) - 1
	add esi, 4
	add eax, edx
	cmp eax, MIXING_CLIPMIN
	jl cliplow
	cmp eax, MIXING_CLIPMAX
	jg cliphigh
	cmp eax, ecx
	jl updatemin
	cmp eax, ebp
	jg updatemax
cliprecover:
	sar eax, 16-MIXING_ATTENUATION
	dec edi
	mov word ptr [ebx-2], ax
	jnz cliploop
	mov edx, ebp
	pop ebp
	mov eax, 28[esp]
	mov dword ptr [eax], ecx
	mov eax, 32[esp]
	mov dword ptr [eax], edx
	mov eax, 24[esp]
	pop edi
	shl eax, 1
	pop esi
	pop ebx
	ret
updatemin:
	mov ecx, eax
	jmp cliprecover
updatemax:
	mov ebp, eax
	jmp cliprecover
cliplow:
	mov ecx, MIXING_CLIPMIN
	mov ebp, MIXING_CLIPMAX
	mov eax, MIXING_CLIPMIN
	jmp cliprecover
cliphigh:
	mov ecx, MIXING_CLIPMIN
	mov ebp, MIXING_CLIPMAX
	mov eax, MIXING_CLIPMAX
	jmp cliprecover
	}
}
#else //WIN32
//---GCCFIX: Asm replaced with C function
// The C version was written by Rani Assaf <rani@magic.metawire.com>, I believe
__declspec(naked) DWORD MPPASMCALL X86_Convert32To16(LPVOID lp16, int *pBuffer, DWORD lSampleCount, LPLONG lpMin, LPLONG lpMax)
{
	int vumin = *lpMin, vumax = *lpMax;
	signed short *p = (signed short *)lp16;
	for (UINT i=0; i<lSampleCount; i++)
	{
		int n = pBuffer[i];
		if (n < MIXING_CLIPMIN)
			n = MIXING_CLIPMIN;
		else if (n > MIXING_CLIPMAX)
			n = MIXING_CLIPMAX;
		if (n < vumin)
			vumin = n;
		else if (n > vumax)
			vumax = n;
		p[i] = n >> (16-MIXING_ATTENUATION);	// 16-bit signed
	}
	*lpMin = vumin;
	*lpMax = vumax;
	return lSampleCount * 2;
}
#endif //WIN32, else

#ifdef WIN32
// Clip and convert to 24 bit
__declspec(naked) DWORD MPPASMCALL X86_Convert32To24(LPVOID lp16, int *pBuffer, DWORD lSampleCount, LPLONG lpMin, LPLONG lpMax)
//-----------------------------------------------------------------------------------------------------------------------------
{
	_asm {
	push ebx
	push esi
	push edi
	mov ebx, 16[esp]		// ebx = 8-bit buffer
	mov esi, 20[esp]		// esi = pBuffer
	mov edi, 24[esp]		// edi = lSampleCount
	mov eax, 28[esp]
	mov ecx, dword ptr [eax]	// ecx = clipmin
	mov eax, 32[esp]
	push ebp
	mov edx, dword ptr [eax]	// edx = clipmax
cliploop:
	mov eax, dword ptr [esi]
	mov ebp, eax
	sar ebp, 31
	and ebp, (1 << (8-MIXING_ATTENUATION)) - 1
	add eax, ebp
	cmp eax, MIXING_CLIPMIN
	jl cliplow
	cmp eax, MIXING_CLIPMAX
	jg cliphigh
	cmp eax, ecx
	jl updatemin
	cmp eax, edx
	jg updatemax
cliprecover:
	add ebx, 3
	sar eax, 8-MIXING_ATTENUATION
	add esi, 4
	mov word ptr [ebx-3], ax
	shr eax, 16
	dec edi
	mov byte ptr [ebx-1], al
	jnz cliploop
	pop ebp
	mov eax, 28[esp]
	mov dword ptr [eax], ecx
	mov eax, 32[esp]
	mov dword ptr [eax], edx
	mov edx, 24[esp]
	mov eax, edx
	pop edi
	shl eax, 1
	pop esi
	add eax, edx
	pop ebx
	ret
updatemin:
	mov ecx, eax
	jmp cliprecover
updatemax:
	mov edx, eax
	jmp cliprecover
cliplow:
	mov ecx, MIXING_CLIPMIN
	mov edx, MIXING_CLIPMAX
	mov eax, MIXING_CLIPMIN
	jmp cliprecover
cliphigh:
	mov ecx, MIXING_CLIPMIN
	mov edx, MIXING_CLIPMAX
	mov eax, MIXING_CLIPMAX
	jmp cliprecover
	}
}
#else //WIN32
//---GCCFIX: Asm replaced with C function
// 24-bit audio not supported.
__declspec(naked) DWORD MPPASMCALL X86_Convert32To24(LPVOID lp16, int *pBuffer, DWORD lSampleCount, LPLONG lpMin, LPLONG lpMax)
{
	return 0;
}
#endif

#ifdef WIN32
// Clip and convert to 32 bit
__declspec(naked) DWORD MPPASMCALL X86_Convert32To32(LPVOID lp16, int *pBuffer, DWORD lSampleCount, LPLONG lpMin, LPLONG lpMax)
//-----------------------------------------------------------------------------------------------------------------------------
{
	_asm {
	push ebx
	push esi
	push edi
	mov ebx, 16[esp]			// ebx = 32-bit buffer
	mov esi, 20[esp]			// esi = pBuffer
	mov edi, 24[esp]			// edi = lSampleCount
	mov eax, 28[esp]
	mov ecx, dword ptr [eax]	// ecx = clipmin
	mov eax, 32[esp]
	mov edx, dword ptr [eax]	// edx = clipmax
cliploop:
	mov eax, dword ptr [esi]
	add ebx, 4
	add esi, 4
	cmp eax, MIXING_CLIPMIN
	jl cliplow
	cmp eax, MIXING_CLIPMAX
	jg cliphigh
	cmp eax, ecx
	jl updatemin
	cmp eax, edx
	jg updatemax
cliprecover:
	shl eax, MIXING_ATTENUATION
	dec edi
	mov dword ptr [ebx-4], eax
	jnz cliploop
	mov eax, 28[esp]
	mov dword ptr [eax], ecx
	mov eax, 32[esp]
	mov dword ptr [eax], edx
	mov edx, 24[esp]
	pop edi
	mov eax, edx
	pop esi
	shl eax, 2
	pop ebx
	ret
updatemin:
	mov ecx, eax
	jmp cliprecover
updatemax:
	mov edx, eax
	jmp cliprecover
cliplow:
	mov ecx, MIXING_CLIPMIN
	mov edx, MIXING_CLIPMAX
	mov eax, MIXING_CLIPMIN
	jmp cliprecover
cliphigh:
	mov ecx, MIXING_CLIPMIN
	mov edx, MIXING_CLIPMAX
	mov eax, MIXING_CLIPMAX
	jmp cliprecover
	}
}
#else
//---GCCFIX: Asm replaced with C function
// 32-bit audio not supported
__declspec(naked) DWORD MPPASMCALL X86_Convert32To32(LPVOID lp16, int *pBuffer, DWORD lSampleCount, LPLONG lpMin, LPLONG lpMax)
{
	return 0;
}
#endif


#ifdef WIN32
void MPPASMCALL X86_InitMixBuffer(int *pBuffer, UINT nSamples)
//------------------------------------------------------------
{
	_asm {
	mov ecx, nSamples
	mov esi, pBuffer
	xor eax, eax
	mov edx, ecx
	shr ecx, 2
	and edx, 3
	jz unroll4x
loop1x:
	add esi, 4
	dec edx
	mov dword ptr [esi-4], eax
	jnz loop1x
unroll4x:
	or ecx, ecx
	jnz loop4x
	jmp done
loop4x:
	add esi, 16
	dec ecx
	mov dword ptr [esi-16], eax
	mov dword ptr [esi-12], eax
	mov dword ptr [esi-8], eax
	mov dword ptr [esi-4], eax
	jnz loop4x
done:;
	}
}
#else
//---GCCFIX: Asm replaced with C function
// Will fill in later.
void MPPASMCALL X86_InitMixBuffer(int *pBuffer, UINT nSamples)
{
	memset(pBuffer, 0, nSamples * sizeof(int));
}
#endif


#ifdef WIN32
__declspec(naked) void MPPASMCALL X86_InterleaveFrontRear(int *pFrontBuf, int *pRearBuf, DWORD nSamples)
//------------------------------------------------------------------------------------------------------
{
	_asm {
	push ebx
	push ebp
	push esi
	push edi
	mov ecx, 28[esp] // ecx = samplecount
	mov esi, 20[esp] // esi = front buffer
	mov edi, 24[esp] // edi = rear buffer
	lea esi, [esi+ecx*4]	// esi = &front[N]
	lea edi, [edi+ecx*4]	// edi = &rear[N]
	lea ebx, [esi+ecx*4]	// ebx = &front[N*2]
interleaveloop:
	mov eax, dword ptr [esi-8]
	mov edx, dword ptr [esi-4]
	sub ebx, 16
	mov ebp, dword ptr [edi-8]
	mov dword ptr [ebx], eax
	mov dword ptr [ebx+4], edx
	mov eax, dword ptr [edi-4]
	sub esi, 8
	sub edi, 8
	dec ecx
	mov dword ptr [ebx+8], ebp
	mov dword ptr [ebx+12], eax
	jnz interleaveloop
	pop edi
	pop esi
	pop ebp
	pop ebx
	ret
	}
}
#else
//---GCCFIX: Asm replaced with C function
// Multichannel not supported.
__declspec(naked) void MPPASMCALL X86_InterleaveFrontRear(int *pFrontBuf, int *pRearBuf, DWORD nSamples)
{
}
#endif


#ifdef WIN32
VOID MPPASMCALL X86_MonoFromStereo(int *pMixBuf, UINT nSamples)
//-------------------------------------------------------------
{
	_asm {
	mov ecx, nSamples
	mov esi, pMixBuf
	mov edi, esi
stloop:
	mov eax, dword ptr [esi]
	mov edx, dword ptr [esi+4]
	add edi, 4
	add esi, 8
	add eax, edx
	sar eax, 1
	dec ecx
	mov dword ptr [edi-4], eax
	jnz stloop
	}
}
#else
//---GCCFIX: Asm replaced with C function
VOID MPPASMCALL X86_MonoFromStereo(int *pMixBuf, UINT nSamples)
{
	UINT j;
	for(UINT i = 0; i < nSamples; i++)
	{
		j = i << 1;
		pMixBuf[i] = (pMixBuf[j] + pMixBuf[j + 1]) >> 1;
	}
}
#endif

#define OFSDECAYSHIFT	8
#define OFSDECAYMASK	0xFF


#ifdef WIN32
void MPPASMCALL X86_StereoFill(int *pBuffer, UINT nSamples, LPLONG lpROfs, LPLONG lpLOfs)
//---------------------------------------------------------------------------------------
{
	_asm {
	mov edi, pBuffer
	mov ecx, nSamples
	mov eax, lpROfs
	mov edx, lpLOfs
	mov eax, [eax]
	mov edx, [edx]
	or ecx, ecx
	jz fill_loop
	mov ebx, eax
	or ebx, edx
	jz fill_loop
ofsloop:
	mov ebx, eax
	mov esi, edx
	neg ebx
	neg esi
	sar ebx, 31
	sar esi, 31
	and ebx, OFSDECAYMASK
	and esi, OFSDECAYMASK
	add ebx, eax
	add esi, edx
	sar ebx, OFSDECAYSHIFT
	sar esi, OFSDECAYSHIFT
	sub eax, ebx
	sub edx, esi
	mov ebx, eax
	or ebx, edx
	jz fill_loop
	add edi, 8
	dec ecx
	mov [edi-8], eax
	mov [edi-4], edx
	jnz ofsloop
fill_loop:
	mov ebx, ecx
	and ebx, 3
	jz fill4x
fill1x:
	mov [edi], eax
	mov [edi+4], edx
	add edi, 8
	dec ebx
	jnz fill1x
fill4x:
	shr ecx, 2
	or ecx, ecx
	jz done
fill4xloop:
	mov [edi], eax
	mov [edi+4], edx
	mov [edi+8], eax
	mov [edi+12], edx
	add edi, 8*4
	dec ecx
	mov [edi-16], eax
	mov [edi-12], edx
	mov [edi-8], eax
	mov [edi-4], edx
	jnz fill4xloop
done:
	mov esi, lpROfs
	mov edi, lpLOfs
	mov [esi], eax
	mov [edi], edx
	}
}
#else
//---GCCFIX: Asm replaced with C function
#define OFSDECAYSHIFT    8
#define OFSDECAYMASK     0xFF
__declspec(naked) void MPPASMCALL X86_StereoFill(int *pBuffer, UINT nSamples, LPLONG lpROfs, LPLONG lpLOfs)
//---------------------------------------------------------------------------------------------------------
{
	int rofs = *lpROfs;
	int lofs = *lpLOfs;
	
	if ((!rofs) && (!lofs))
	{
		X86_InitMixBuffer(pBuffer, nSamples*2);
		return;
	}
	for (UINT i=0; i<nSamples; i++)
	{
		int x_r = (rofs + (((-rofs)>>31) & OFSDECAYMASK)) >> OFSDECAYSHIFT;
		int x_l = (lofs + (((-lofs)>>31) & OFSDECAYMASK)) >> OFSDECAYSHIFT;
		rofs -= x_r;
		lofs -= x_l;
		pBuffer[i*2] = x_r;
		pBuffer[i*2+1] = x_l;
	}
	*lpROfs = rofs;
	*lpLOfs = lofs;
}
#endif

#ifdef WIN32
void MPPASMCALL X86_EndChannelOfs(MODCHANNEL *pChannel, int *pBuffer, UINT nSamples)
//----------------------------------------------------------------------------------
{
	_asm {
	mov esi, pChannel
	mov edi, pBuffer
	mov ecx, nSamples
	mov eax, dword ptr [esi+MODCHANNEL.nROfs]
	mov edx, dword ptr [esi+MODCHANNEL.nLOfs]
	or ecx, ecx
	jz brkloop
ofsloop:
	mov ebx, eax
	mov esi, edx
	neg ebx
	neg esi
	sar ebx, 31
	sar esi, 31
	and ebx, OFSDECAYMASK
	and esi, OFSDECAYMASK
	add ebx, eax
	add esi, edx
	sar ebx, OFSDECAYSHIFT
	sar esi, OFSDECAYSHIFT
	sub eax, ebx
	sub edx, esi
	mov ebx, eax
	add dword ptr [edi], eax
	add dword ptr [edi+4], edx
	or ebx, edx
	jz brkloop
	add edi, 8
	dec ecx
	jnz ofsloop
brkloop:
	mov esi, pChannel
	mov dword ptr [esi+MODCHANNEL.nROfs], eax
	mov dword ptr [esi+MODCHANNEL.nLOfs], edx
	}
}
#else
//---GCCFIX: Asm replaced with C function
// Will fill in later.
void MPPASMCALL X86_EndChannelOfs(MODCHANNEL *pChannel, int *pBuffer, UINT nSamples)
{
	int rofs = pChannel->nROfs;
	int lofs = pChannel->nLOfs;
	
	if ((!rofs) && (!lofs)) return;
	for (UINT i=0; i<nSamples; i++)
	{
		int x_r = (rofs + (((-rofs)>>31) & OFSDECAYMASK)) >> OFSDECAYSHIFT;
		int x_l = (lofs + (((-lofs)>>31) & OFSDECAYMASK)) >> OFSDECAYSHIFT;
		rofs -= x_r;
		lofs -= x_l;
		pBuffer[i*2] += x_r;
		pBuffer[i*2+1] += x_l;
	}
	pChannel->nROfs = rofs;
	pChannel->nLOfs = lofs;
}
#endif


//////////////////////////////////////////////////////////////////////////////////
// Automatic Gain Control

#ifndef NO_AGC

// Limiter
#define MIXING_LIMITMAX		(0x08100000)
#define MIXING_LIMITMIN		(-MIXING_LIMITMAX)

__declspec(naked) UINT MPPASMCALL X86_AGC(int *pBuffer, UINT nSamples, UINT nAGC)
//-------------------------------------------------------------------------------
{
	__asm {
	push ebx
	push ebp
	push esi
	push edi
	mov esi, 20[esp]	// esi = pBuffer+i
	mov ecx, 24[esp]	// ecx = i
	mov edi, 28[esp]	// edi = AGC (0..256)
agcloop:
	mov eax, dword ptr [esi]
	imul edi
	shrd eax, edx, AGC_PRECISION
	add esi, 4
	cmp eax, MIXING_LIMITMIN
	jl agcupdate
	cmp eax, MIXING_LIMITMAX
	jg agcupdate
agcrecover:
	dec ecx
	mov dword ptr [esi-4], eax
	jnz agcloop
	mov eax, edi
	pop edi
	pop esi
	pop ebp
	pop ebx
	ret
agcupdate:
	dec edi
	jmp agcrecover
	}
}

#pragma warning (default:4100)

void CSoundFile::ProcessAGC(int count)
//------------------------------------
{
	static DWORD gAGCRecoverCount = 0;
	UINT agc = X86_AGC(MixSoundBuffer, count, gnAGC);
	// Some kind custom law, so that the AGC stays quite stable, but slowly
	// goes back up if the sound level stays below a level inversely proportional
	// to the AGC level. (J'me comprends)
	if ((agc >= gnAGC) && (gnAGC < AGC_UNITY) && (gnVUMeter < (0xFF - (gnAGC >> (AGC_PRECISION-7))) ))
	{
		gAGCRecoverCount += count;
		UINT agctimeout = gdwMixingFreq + gnAGC;
		if (gnChannels >= 2) agctimeout <<= 1;
		if (gAGCRecoverCount >= agctimeout)
		{
			gAGCRecoverCount = 0;
			gnAGC++;
		}
	} else
	{
		gnAGC = agc;
		gAGCRecoverCount = 0;
	}
}



void CSoundFile::ResetAGC()
//-------------------------
{
	gnAGC = AGC_UNITY;
}

#endif // NO_AGC




