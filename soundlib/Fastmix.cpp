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
#ifdef _DEBUG
#include <math.h>
#endif

// rewbs.resamplerConf
#include "../mptrack/mptrack.h"
#include "../mptrack/MainFrm.h"
#include "WindowedFIR.h"
// end  rewbs.resamplerConf

#pragma bss_seg(".modplug")

// Front Mix Buffer (Also room for interleaved rear mix)
int MixSoundBuffer[MIXBUFFERSIZE*4];

// Reverb Mix Buffer
#ifndef NO_REVERB
int MixReverbBuffer[MIXBUFFERSIZE*2];
#endif

#ifndef FASTSOUNDLIB
int MixRearBuffer[MIXBUFFERSIZE*2];
float MixFloatBuffer[MIXBUFFERSIZE*2];
#endif

#pragma bss_seg()


#ifndef NO_REVERB
extern UINT gnReverbSend;
#endif

extern LONG gnDryROfsVol;
extern LONG gnDryLOfsVol;
extern LONG gnRvbROfsVol;
extern LONG gnRvbLOfsVol;

// 4x256 taps polyphase FIR resampling filter
extern short int gFastSinc[];
extern short int gKaiserSinc[]; // 8-taps polyphase
extern short int gDownsample13x[]; // 1.3x downsampling
extern short int gDownsample2x[]; // 2x downsampling


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

signed short CWindowedFIR::lut[WFIR_LUTLEN*WFIR_WIDTH]; // rewbs.resamplerConf
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

// Cubic Spline
#define SNDMIX_GETMONOVOL8HQSRC\
	int poshi = nPos >> 16;\
	int poslo = (nPos >> 6) & 0x3FC;\
	int vol = (gFastSinc[poslo]*p[poshi-1] + gFastSinc[poslo+1]*p[poshi]\
		 + gFastSinc[poslo+2]*p[poshi+1] + gFastSinc[poslo+3]*p[poshi+2]) >> 6;\

#define SNDMIX_GETMONOVOL16HQSRC\
	int poshi = nPos >> 16;\
	int poslo = (nPos >> 6) & 0x3FC;\
	int vol = (gFastSinc[poslo]*p[poshi-1] + gFastSinc[poslo+1]*p[poshi]\
		 + gFastSinc[poslo+2]*p[poshi+1] + gFastSinc[poslo+3]*p[poshi+2]) >> 14;\

// 8-taps polyphase
#define SNDMIX_GETMONOVOL8KAISER\
	int poshi = nPos >> 16;\
	const short int *poslo = (const short int *)(sinc+(nPos&0xfff0));\
	int vol = (poslo[0]*p[poshi-3] + poslo[1]*p[poshi-2]\
		 + poslo[2]*p[poshi-1] + poslo[3]*p[poshi]\
		 + poslo[4]*p[poshi+1] + poslo[5]*p[poshi+2]\
		 + poslo[6]*p[poshi+3] + poslo[7]*p[poshi+4]) >> 6;\

#define SNDMIX_GETMONOVOL16KAISER\
	int poshi = nPos >> 16;\
	const short int *poslo = (const short int *)(sinc+(nPos&0xfff0));\
	int vol = (poslo[0]*p[poshi-3] + poslo[1]*p[poshi-2]\
		 + poslo[2]*p[poshi-1] + poslo[3]*p[poshi]\
		 + poslo[4]*p[poshi+1] + poslo[5]*p[poshi+2]\
		 + poslo[6]*p[poshi+3] + poslo[7]*p[poshi+4]) >> 14;\
// rewbs.resamplerConf
#define SNDMIX_GETMONOVOL8FIRFILTER \
	int poshi  = nPos >> 16;\
	int poslo  = (nPos & 0xFFFF);\
	int firidx = ((poslo+WFIR_FRACHALVE)>>WFIR_FRACSHIFT) & WFIR_FRACMASK; \
	int vol    = (CWindowedFIR::lut[firidx+0]*(int)p[poshi+1-4]);	\
        vol   += (CWindowedFIR::lut[firidx+1]*(int)p[poshi+2-4]);	\
        vol   += (CWindowedFIR::lut[firidx+2]*(int)p[poshi+3-4]);	\
        vol   += (CWindowedFIR::lut[firidx+3]*(int)p[poshi+4-4]);	\
        vol   += (CWindowedFIR::lut[firidx+4]*(int)p[poshi+5-4]);	\
        vol   += (CWindowedFIR::lut[firidx+5]*(int)p[poshi+6-4]);	\
        vol   += (CWindowedFIR::lut[firidx+6]*(int)p[poshi+7-4]);	\
        vol   += (CWindowedFIR::lut[firidx+7]*(int)p[poshi+8-4]);	\
        vol  >>= WFIR_8SHIFT;

#define SNDMIX_GETMONOVOL16FIRFILTER \
    int poshi  = nPos >> 16;\
    int poslo  = (nPos & 0xFFFF);\
    int firidx = ((poslo+WFIR_FRACHALVE)>>WFIR_FRACSHIFT) & WFIR_FRACMASK; \
    int vol1   = (CWindowedFIR::lut[firidx+0]*(int)p[poshi+1-4]);	\
        vol1  += (CWindowedFIR::lut[firidx+1]*(int)p[poshi+2-4]);	\
        vol1  += (CWindowedFIR::lut[firidx+2]*(int)p[poshi+3-4]);	\
        vol1  += (CWindowedFIR::lut[firidx+3]*(int)p[poshi+4-4]);	\
    int vol2   = (CWindowedFIR::lut[firidx+4]*(int)p[poshi+5-4]);	\
		vol2  += (CWindowedFIR::lut[firidx+5]*(int)p[poshi+6-4]);	\
		vol2  += (CWindowedFIR::lut[firidx+6]*(int)p[poshi+7-4]);	\
		vol2  += (CWindowedFIR::lut[firidx+7]*(int)p[poshi+8-4]);	\
    int vol    = ((vol1>>1)+(vol2>>1)) >> (WFIR_16BITSHIFT-1);


// end rewbs.resamplerConf
#define SNDMIX_INITSINCTABLE\
	const char * const sinc = (const char *)(((pChannel->nInc > 0x13000) || (pChannel->nInc < -0x13000)) ?\
		(((pChannel->nInc > 0x18000) || (pChannel->nInc < -0x18000)) ? gDownsample2x : gDownsample13x) : gKaiserSinc);

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

// Cubic Spline
#define SNDMIX_GETSTEREOVOL8HQSRC\
	int poshi = nPos >> 16;\
	int poslo = (nPos >> 6) & 0x3FC;\
	int vol_l = (gFastSinc[poslo]*p[poshi*2-2] + gFastSinc[poslo+1]*p[poshi*2]\
		 + gFastSinc[poslo+2]*p[poshi*2+2] + gFastSinc[poslo+3]*p[poshi*2+4]) >> 6;\
	int vol_r = (gFastSinc[poslo]*p[poshi*2-1] + gFastSinc[poslo+1]*p[poshi*2+1]\
		 + gFastSinc[poslo+2]*p[poshi*2+3] + gFastSinc[poslo+3]*p[poshi*2+5]) >> 6;\

#define SNDMIX_GETSTEREOVOL16HQSRC\
	int poshi = nPos >> 16;\
	int poslo = (nPos >> 6) & 0x3FC;\
	int vol_l = (gFastSinc[poslo]*p[poshi*2-2] + gFastSinc[poslo+1]*p[poshi*2]\
		 + gFastSinc[poslo+2]*p[poshi*2+2] + gFastSinc[poslo+3]*p[poshi*2+4]) >> 14;\
	int vol_r = (gFastSinc[poslo]*p[poshi*2-1] + gFastSinc[poslo+1]*p[poshi*2+1]\
		 + gFastSinc[poslo+2]*p[poshi*2+3] + gFastSinc[poslo+3]*p[poshi*2+5]) >> 14;\

// -> CODE#0025
// -> DESC="enable polyphase resampling on stereo samples"
// 8-taps polyphase
#define SNDMIX_GETSTEREOVOL8KAISER\
	int poshi = nPos >> 16;\
	const short int *poslo = (const short int *)(sinc+(nPos&0xfff0));\
	int vol_l = (poslo[0]*p[poshi*2-6] + poslo[1]*p[poshi*2-4]\
		 + poslo[2]*p[poshi*2-2] + poslo[3]*p[poshi*2]\
		 + poslo[4]*p[poshi*2+2] + poslo[5]*p[poshi*2+4]\
		 + poslo[6]*p[poshi*2+6] + poslo[7]*p[poshi*2+8]) >> 6;\
	int vol_r = (poslo[0]*p[poshi*2-5] + poslo[1]*p[poshi*2-3]\
		 + poslo[2]*p[poshi*2-1] + poslo[3]*p[poshi*2+1]\
		 + poslo[4]*p[poshi*2+3] + poslo[5]*p[poshi*2+5]\
		 + poslo[6]*p[poshi*2+7] + poslo[7]*p[poshi*2+9]) >> 6;\

#define SNDMIX_GETSTEREOVOL16KAISER\
	int poshi = nPos >> 16;\
	const short int *poslo = (const short int *)(sinc+(nPos&0xfff0));\
	int vol_l = (poslo[0]*p[poshi*2-6] + poslo[1]*p[poshi*2-4]\
		 + poslo[2]*p[poshi*2-2] + poslo[3]*p[poshi*2]\
		 + poslo[4]*p[poshi*2+2] + poslo[5]*p[poshi*2+4]\
		 + poslo[6]*p[poshi*2+6] + poslo[7]*p[poshi*2+8]) >> 14;\
	int vol_r = (poslo[0]*p[poshi*2-5] + poslo[1]*p[poshi*2-3]\
		 + poslo[2]*p[poshi*2-1] + poslo[3]*p[poshi*2+1]\
		 + poslo[4]*p[poshi*2+3] + poslo[5]*p[poshi*2+5]\
		 + poslo[6]*p[poshi*2+7] + poslo[7]*p[poshi*2+9]) >> 14;\
// rewbs.resamplerConf
// fir interpolation
#define SNDMIX_GETSTEREOVOL8FIRFILTER \
    int poshi   = nPos >> 16;\
    int poslo   = (nPos & 0xFFFF);\
    int firidx  = ((poslo+WFIR_FRACHALVE)>>WFIR_FRACSHIFT) & WFIR_FRACMASK; \
    int vol_l   = (CWindowedFIR::lut[firidx+0]*(int)p[(poshi+1-4)*2  ]);   \
		vol_l  += (CWindowedFIR::lut[firidx+1]*(int)p[(poshi+2-4)*2  ]);   \
		vol_l  += (CWindowedFIR::lut[firidx+2]*(int)p[(poshi+3-4)*2  ]);   \
        vol_l  += (CWindowedFIR::lut[firidx+3]*(int)p[(poshi+4-4)*2  ]);   \
        vol_l  += (CWindowedFIR::lut[firidx+4]*(int)p[(poshi+5-4)*2  ]);   \
		vol_l  += (CWindowedFIR::lut[firidx+5]*(int)p[(poshi+6-4)*2  ]);   \
		vol_l  += (CWindowedFIR::lut[firidx+6]*(int)p[(poshi+7-4)*2  ]);   \
        vol_l  += (CWindowedFIR::lut[firidx+7]*(int)p[(poshi+8-4)*2  ]);   \
		vol_l >>= WFIR_8SHIFT; \
    int vol_r   = (CWindowedFIR::lut[firidx+0]*(int)p[(poshi+1-4)*2+1]);   \
		vol_r  += (CWindowedFIR::lut[firidx+1]*(int)p[(poshi+2-4)*2+1]);   \
		vol_r  += (CWindowedFIR::lut[firidx+2]*(int)p[(poshi+3-4)*2+1]);   \
		vol_r  += (CWindowedFIR::lut[firidx+3]*(int)p[(poshi+4-4)*2+1]);   \
		vol_r  += (CWindowedFIR::lut[firidx+4]*(int)p[(poshi+5-4)*2+1]);   \
        vol_r  += (CWindowedFIR::lut[firidx+5]*(int)p[(poshi+6-4)*2+1]);   \
        vol_r  += (CWindowedFIR::lut[firidx+6]*(int)p[(poshi+7-4)*2+1]);   \
        vol_r  += (CWindowedFIR::lut[firidx+7]*(int)p[(poshi+8-4)*2+1]);   \
        vol_r >>= WFIR_8SHIFT;

#define SNDMIX_GETSTEREOVOL16FIRFILTER \
    int poshi   = nPos >> 16;\
    int poslo   = (nPos & 0xFFFF);\
    int firidx  = ((poslo+WFIR_FRACHALVE)>>WFIR_FRACSHIFT) & WFIR_FRACMASK; \
    int vol1_l  = (CWindowedFIR::lut[firidx+0]*(int)p[(poshi+1-4)*2  ]);   \
		vol1_l += (CWindowedFIR::lut[firidx+1]*(int)p[(poshi+2-4)*2  ]);   \
        vol1_l += (CWindowedFIR::lut[firidx+2]*(int)p[(poshi+3-4)*2  ]);   \
		vol1_l += (CWindowedFIR::lut[firidx+3]*(int)p[(poshi+4-4)*2  ]);   \
	int vol2_l  = (CWindowedFIR::lut[firidx+4]*(int)p[(poshi+5-4)*2  ]);   \
		vol2_l += (CWindowedFIR::lut[firidx+5]*(int)p[(poshi+6-4)*2  ]);   \
		vol2_l += (CWindowedFIR::lut[firidx+6]*(int)p[(poshi+7-4)*2  ]);   \
		vol2_l += (CWindowedFIR::lut[firidx+7]*(int)p[(poshi+8-4)*2  ]);   \
	int vol_l   = ((vol1_l>>1)+(vol2_l>>1)) >> (WFIR_16BITSHIFT-1); \
	int vol1_r  = (CWindowedFIR::lut[firidx+0]*(int)p[(poshi+1-4)*2+1]);   \
		vol1_r += (CWindowedFIR::lut[firidx+1]*(int)p[(poshi+2-4)*2+1]);   \
		vol1_r += (CWindowedFIR::lut[firidx+2]*(int)p[(poshi+3-4)*2+1]);   \
		vol1_r += (CWindowedFIR::lut[firidx+3]*(int)p[(poshi+4-4)*2+1]);   \
	int vol2_r  = (CWindowedFIR::lut[firidx+4]*(int)p[(poshi+5-4)*2+1]);   \
		vol2_r += (CWindowedFIR::lut[firidx+5]*(int)p[(poshi+6-4)*2+1]);   \
		vol2_r += (CWindowedFIR::lut[firidx+6]*(int)p[(poshi+7-4)*2+1]);   \
		vol2_r += (CWindowedFIR::lut[firidx+7]*(int)p[(poshi+8-4)*2+1]);   \
	int vol_r   = ((vol1_r>>1)+(vol2_r>>1)) >> (WFIR_16BITSHIFT-1);
//end rewbs.resamplerConf
// -! BEHAVIOUR_CHANGE#0025


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
	int fy = (vol * pChn->nFilter_A0 + fy1 * pChn->nFilter_B0 + fy2 * pChn->nFilter_B1 + 4096) >> 13;\
	fy2 = fy1;\
	fy1 = fy - (vol & pChn->nFilter_HP);\
	vol = fy;\
	

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
	int fy = (vol_l * pChn->nFilter_A0 + fy1 * pChn->nFilter_B0 + fy2 * pChn->nFilter_B1 + 4096) >> 13;\
	fy2 = fy1; fy1 = fy - (vol_l & pChn->nFilter_HP);\
	vol_l = fy;\
	fy = (vol_r * pChn->nFilter_A0 + fy3 * pChn->nFilter_B0 + fy4 * pChn->nFilter_B1 + 4096) >> 13;\
	fy4 = fy3; fy3 = fy - (vol_r & pChn->nFilter_HP);\
	vol_r = fy;\


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
extern void X86_StereoMixToFloat(const int *pSrc, float *pOut1, float *pOut2, UINT nCount, const float _i2fc);
extern void X86_FloatToStereoMix(const float *pIn1, const float *pIn2, int *pOut, UINT nCount, const float _f2ic);
extern void X86_MonoMixToFloat(const int *pSrc, float *pOut, UINT nCount, const float _i2fc);
extern void X86_FloatToMonoMix(const float *pIn, int *pOut, UINT nCount, const float _f2ic);

void MPPASMCALL X86_InitMixBuffer(int *pBuffer, UINT nSamples);
void MPPASMCALL X86_EndChannelOfs(MODCHANNEL *pChannel, int *pBuffer, UINT nSamples);
void MPPASMCALL X86_StereoFill(int *pBuffer, UINT nSamples, LPLONG lpROfs, LPLONG lpLOfs);

#ifdef ENABLE_MMX
extern VOID MMX_EndMix();
extern VOID MMX_Mono8BitMix(MODCHANNEL *, int *, int *);
extern VOID MMX_Mono16BitMix(MODCHANNEL *, int *, int *);
extern VOID MMX_Mono8BitLinearMix(MODCHANNEL *, int *, int *);
extern VOID MMX_Mono16BitLinearMix(MODCHANNEL *, int *, int *);
extern VOID MMX_Mono8BitHQMix(MODCHANNEL *, int *, int *);
extern VOID MMX_Mono16BitHQMix(MODCHANNEL *, int *, int *);
extern VOID MMX_Mono8BitKaiserMix(MODCHANNEL *, int *, int *);
extern VOID MMX_Mono16BitKaiserMix(MODCHANNEL *, int *, int *);
extern VOID MMX_Mono8BitKaiserRampMix(MODCHANNEL *, int *, int *);
extern VOID MMX_Mono16BitKaiserRampMix(MODCHANNEL *, int *, int *);
extern VOID MMX_FilterMono8BitLinearRampMix(MODCHANNEL *, int *, int *);
extern VOID MMX_FilterMono16BitLinearRampMix(MODCHANNEL *, int *, int *);
#define MMX_FilterMono8BitMix			MMX_FilterMono8BitLinearRampMix
#define MMX_FilterMono16BitMix			MMX_FilterMono16BitLinearRampMix
#define MMX_FilterMono8BitRampMix		MMX_FilterMono8BitLinearRampMix
#define MMX_FilterMono16BitRampMix		MMX_FilterMono16BitLinearRampMix
#define MMX_FilterMono8BitLinearMix		MMX_FilterMono8BitLinearRampMix
#define MMX_FilterMono16BitLinearMix	MMX_FilterMono16BitLinearRampMix
#endif


#ifdef ENABLE_AMD
extern void AMD_StereoMixToFloat(const int *pSrc, float *pOut1, float *pOut2, UINT nCount, const float _i2fc);
extern void AMD_FloatToStereoMix(const float *pIn1, const float *pIn2, int *pOut, UINT nCount, const float _f2ic);
extern void AMD_MonoMixToFloat(const int *pSrc, float *pOut, UINT nCount, const float _i2fc);
extern void AMD_FloatToMonoMix(const float *pIn, int *pOut, UINT nCount, const float _f2ic);
#endif

#ifdef ENABLE_SSE
extern void SSE_StereoMixToFloat(const int *pSrc, float *pOut1, float *pOut2, UINT nCount, const float _i2fc);
extern void SSE_MonoMixToFloat(const int *pSrc, float *pOut, UINT nCount, const float _i2fc);
#endif


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

#ifndef FASTSOUNDLIB
BEGIN_MIX_INTERFACE(Mono8BitHQMix)
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETMONOVOL8HQSRC
	SNDMIX_STOREMONOVOL
END_MIX_INTERFACE()

BEGIN_MIX_INTERFACE(Mono16BitHQMix)
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETMONOVOL16HQSRC
	SNDMIX_STOREMONOVOL
END_MIX_INTERFACE()
#else
#define Mono8BitHQMix	Mono8BitLinearMix
#define Mono16BitHQMix	Mono16BitLinearMix
#endif

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

#ifndef FASTSOUNDLIB
BEGIN_RAMPMIX_INTERFACE(Mono8BitHQRampMix)
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETMONOVOL8HQSRC
	SNDMIX_RAMPMONOVOL
END_RAMPMIX_INTERFACE()

BEGIN_RAMPMIX_INTERFACE(Mono16BitHQRampMix)
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETMONOVOL16HQSRC
	SNDMIX_RAMPMONOVOL
END_RAMPMIX_INTERFACE()

#else
#define	Mono8BitHQRampMix	Mono8BitLinearRampMix
#define	Mono16BitHQRampMix	Mono16BitLinearRampMix
#endif

//////////////////////////////////////////////////////
// 8-taps polyphase resampling filter

#ifndef FASTSOUNDLIB

// Normal
BEGIN_MIX_INTERFACE(Mono8BitKaiserMix)
	SNDMIX_INITSINCTABLE
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETMONOVOL8KAISER
	SNDMIX_STOREMONOVOL
END_MIX_INTERFACE()

BEGIN_MIX_INTERFACE(Mono16BitKaiserMix)
	SNDMIX_INITSINCTABLE
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETMONOVOL16KAISER
	SNDMIX_STOREMONOVOL
END_MIX_INTERFACE()

// Ramp
BEGIN_RAMPMIX_INTERFACE(Mono8BitKaiserRampMix)
	SNDMIX_INITSINCTABLE
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETMONOVOL8KAISER
	SNDMIX_RAMPMONOVOL
END_RAMPMIX_INTERFACE()

BEGIN_RAMPMIX_INTERFACE(Mono16BitKaiserRampMix)
	SNDMIX_INITSINCTABLE
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETMONOVOL16KAISER
	SNDMIX_RAMPMONOVOL
END_RAMPMIX_INTERFACE()


#else
#define Mono8BitKaiserMix		Mono8BitHQMix
#define Mono16BitKaiserMix		Mono16BitHQMix
#define Mono8BitKaiserRampMix	Mono8BitHQRampMix
#define Mono16BitKaiserRampMix	Mono16BitHQRampMix
#endif

// -> BEHAVIOUR_CHANGE#0025
// -> DESC="enable polyphase resampling on stereo samples"
// rewbs.resamplerConf
//////////////////////////////////////////////////////
// FIR filter

#ifndef FASTSOUNDLIB

// Normal
BEGIN_MIX_INTERFACE(Mono8BitFIRFilterMix)
	//SNDMIX_INITSINCTABLE
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETMONOVOL8FIRFILTER
	SNDMIX_STOREMONOVOL
END_MIX_INTERFACE()

BEGIN_MIX_INTERFACE(Mono16BitFIRFilterMix)
	//SNDMIX_INITSINCTABLE
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETMONOVOL16FIRFILTER
	SNDMIX_STOREMONOVOL
END_MIX_INTERFACE()

// Ramp
BEGIN_RAMPMIX_INTERFACE(Mono8BitFIRFilterRampMix)
	//SNDMIX_INITSINCTABLE
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETMONOVOL8FIRFILTER
	SNDMIX_RAMPMONOVOL
END_RAMPMIX_INTERFACE()

BEGIN_RAMPMIX_INTERFACE(Mono16BitFIRFilterRampMix)
	//SNDMIX_INITSINCTABLE
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETMONOVOL16FIRFILTER
	SNDMIX_RAMPMONOVOL
END_RAMPMIX_INTERFACE()


#else
#define Mono8BitFIRFilterMix		Mono8BitHQMix
#define Mono16BitFIRFilterMix		Mono16BitHQMix
#define Mono8BitFIRFilterRampMix	Mono8BitHQRampMix
#define Mono16BitFIRFilterRampMix	Mono16BitHQRampMix
#endif
//end rewbs.resamplerConf
// -! BEHAVIOUR_CHANGE#0025

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

BEGIN_MIX_INTERFACE(Stereo8BitHQMix)
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETSTEREOVOL8HQSRC
	SNDMIX_STORESTEREOVOL
END_MIX_INTERFACE()

BEGIN_MIX_INTERFACE(Stereo16BitHQMix)
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETSTEREOVOL16HQSRC
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

BEGIN_RAMPMIX_INTERFACE(Stereo8BitHQRampMix)
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETSTEREOVOL8HQSRC
	SNDMIX_RAMPSTEREOVOL
END_RAMPMIX_INTERFACE()

BEGIN_RAMPMIX_INTERFACE(Stereo16BitHQRampMix)
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETSTEREOVOL16HQSRC
	SNDMIX_RAMPSTEREOVOL
END_RAMPMIX_INTERFACE()

// -> CODE#0025
// -> DESC="enable polyphase resampling on stereo samples"

//////////////////////////////////////////////////////
// Stereo 8-taps polyphase resampling filter

BEGIN_MIX_INTERFACE(Stereo8BitKaiserMix)
	SNDMIX_INITSINCTABLE
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETSTEREOVOL8KAISER
	SNDMIX_STORESTEREOVOL
END_MIX_INTERFACE()

BEGIN_MIX_INTERFACE(Stereo16BitKaiserMix)
	SNDMIX_INITSINCTABLE
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETSTEREOVOL16KAISER
	SNDMIX_STORESTEREOVOL
END_MIX_INTERFACE()

// Ramp
BEGIN_RAMPMIX_INTERFACE(Stereo8BitKaiserRampMix)
	SNDMIX_INITSINCTABLE
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETSTEREOVOL8KAISER
	SNDMIX_RAMPSTEREOVOL
END_RAMPMIX_INTERFACE()

BEGIN_RAMPMIX_INTERFACE(Stereo16BitKaiserRampMix)
	SNDMIX_INITSINCTABLE
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETSTEREOVOL16KAISER
	SNDMIX_RAMPSTEREOVOL
END_RAMPMIX_INTERFACE()

// rewbs.resamplerConf
//////////////////////////////////////////////////////
// Stereo FIR Filter

BEGIN_MIX_INTERFACE(Stereo8BitFIRFilterMix)
	//SNDMIX_INITSINCTABLE
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETSTEREOVOL8FIRFILTER
	SNDMIX_STORESTEREOVOL
END_MIX_INTERFACE()

BEGIN_MIX_INTERFACE(Stereo16BitFIRFilterMix)
	//SNDMIX_INITSINCTABLE
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETSTEREOVOL16FIRFILTER
	SNDMIX_STORESTEREOVOL
END_MIX_INTERFACE()

// Ramp
BEGIN_RAMPMIX_INTERFACE(Stereo8BitFIRFilterRampMix)
	//SNDMIX_INITSINCTABLE
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETSTEREOVOL8FIRFILTER
	SNDMIX_RAMPSTEREOVOL
END_RAMPMIX_INTERFACE()

BEGIN_RAMPMIX_INTERFACE(Stereo16BitFIRFilterRampMix)
	//SNDMIX_INITSINCTABLE
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETSTEREOVOL16FIRFILTER
	SNDMIX_RAMPSTEREOVOL
END_RAMPMIX_INTERFACE()

// end rewbs.resamplerConf
// -! BEHAVIOUR_CHANGE#0025



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

// -> CODE#0025
// -> DESC="enable polyphase resampling on stereo samples"
// rewbs.resamplerConf
//Cubic + reso filter:
BEGIN_MIX_FLT_INTERFACE(FilterMono8BitHQMix)
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETMONOVOL8HQSRC
	SNDMIX_PROCESSFILTER
	SNDMIX_STOREMONOVOL
END_MIX_FLT_INTERFACE()

BEGIN_MIX_FLT_INTERFACE(FilterMono16BitHQMix)
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETMONOVOL16HQSRC
	SNDMIX_PROCESSFILTER
	SNDMIX_STOREMONOVOL
END_MIX_FLT_INTERFACE()

//Polyphase + reso filter:
BEGIN_MIX_FLT_INTERFACE(FilterMono8BitKaiserMix)
	SNDMIX_INITSINCTABLE
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETMONOVOL8KAISER
	SNDMIX_PROCESSFILTER
	SNDMIX_STOREMONOVOL
END_MIX_FLT_INTERFACE()

BEGIN_MIX_FLT_INTERFACE(FilterMono16BitKaiserMix)
	SNDMIX_INITSINCTABLE
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETMONOVOL16KAISER
	SNDMIX_PROCESSFILTER
	SNDMIX_STOREMONOVOL
END_MIX_FLT_INTERFACE()

// Enable FIR Filter with resonant filters

BEGIN_MIX_FLT_INTERFACE(FilterMono8BitFIRFilterMix)
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETMONOVOL8FIRFILTER
	SNDMIX_PROCESSFILTER
	SNDMIX_STOREMONOVOL
END_MIX_FLT_INTERFACE()

BEGIN_MIX_FLT_INTERFACE(FilterMono16BitFIRFilterMix)
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETMONOVOL16FIRFILTER
	SNDMIX_PROCESSFILTER
	SNDMIX_STOREMONOVOL
END_MIX_FLT_INTERFACE()
// end rewbs.resamplerConf
// -! BEHAVIOUR_CHANGE#0025

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

// -> CODE#0025
// -> DESC="enable polyphase resampling on stereo samples"
// rewbs.resamplerConf
//Cubic + reso filter + ramp:
BEGIN_RAMPMIX_FLT_INTERFACE(FilterMono8BitHQRampMix)
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETMONOVOL8HQSRC
	SNDMIX_PROCESSFILTER
	SNDMIX_RAMPMONOVOL
END_RAMPMIX_FLT_INTERFACE()

BEGIN_RAMPMIX_FLT_INTERFACE(FilterMono16BitHQRampMix)
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETMONOVOL16HQSRC
	SNDMIX_PROCESSFILTER
	SNDMIX_RAMPMONOVOL
END_RAMPMIX_FLT_INTERFACE()

//Polyphase + reso filter + ramp:
BEGIN_RAMPMIX_FLT_INTERFACE(FilterMono8BitKaiserRampMix)
	SNDMIX_INITSINCTABLE
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETMONOVOL8KAISER
	SNDMIX_PROCESSFILTER
	SNDMIX_RAMPMONOVOL
END_RAMPMIX_FLT_INTERFACE()

BEGIN_RAMPMIX_FLT_INTERFACE(FilterMono16BitKaiserRampMix)
	SNDMIX_INITSINCTABLE
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETMONOVOL16KAISER
	SNDMIX_PROCESSFILTER
	SNDMIX_RAMPMONOVOL
END_RAMPMIX_FLT_INTERFACE()

//FIR Filter + reso filter + ramp
BEGIN_RAMPMIX_FLT_INTERFACE(FilterMono8BitFIRFilterRampMix)
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETMONOVOL8FIRFILTER
	SNDMIX_PROCESSFILTER
	SNDMIX_RAMPMONOVOL
END_RAMPMIX_FLT_INTERFACE()

BEGIN_RAMPMIX_FLT_INTERFACE(FilterMono16BitFIRFilterRampMix)
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETMONOVOL16FIRFILTER
	SNDMIX_PROCESSFILTER
	SNDMIX_RAMPMONOVOL
END_RAMPMIX_FLT_INTERFACE()

// end rewbs.resamplerConf
// -! BEHAVIOUR_CHANGE#0025

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

// -> CODE#0025
// -> DESC="enable polyphase resampling on stereo samples"
// rewbs.resamplerConf
//Cubic stereo + reso filter
BEGIN_MIX_STFLT_INTERFACE(FilterStereo8BitHQMix)
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETSTEREOVOL8HQSRC
	SNDMIX_PROCESSSTEREOFILTER
	SNDMIX_STORESTEREOVOL
END_MIX_STFLT_INTERFACE()

BEGIN_MIX_STFLT_INTERFACE(FilterStereo16BitHQMix)
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETSTEREOVOL16HQSRC
	SNDMIX_PROCESSSTEREOFILTER
	SNDMIX_STORESTEREOVOL
END_MIX_STFLT_INTERFACE()

//Polyphase stereo + reso filter
BEGIN_MIX_STFLT_INTERFACE(FilterStereo8BitKaiserMix)
	SNDMIX_INITSINCTABLE
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETSTEREOVOL8KAISER
	SNDMIX_PROCESSSTEREOFILTER
	SNDMIX_STORESTEREOVOL
END_MIX_STFLT_INTERFACE()

BEGIN_MIX_STFLT_INTERFACE(FilterStereo16BitKaiserMix)
	SNDMIX_INITSINCTABLE
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETSTEREOVOL16KAISER
	SNDMIX_PROCESSSTEREOFILTER
	SNDMIX_STORESTEREOVOL
END_MIX_STFLT_INTERFACE()

//FIR filter stereo + reso filter
BEGIN_MIX_STFLT_INTERFACE(FilterStereo8BitFIRFilterMix)
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETSTEREOVOL8FIRFILTER
	SNDMIX_PROCESSSTEREOFILTER
	SNDMIX_STORESTEREOVOL
END_MIX_STFLT_INTERFACE()

BEGIN_MIX_STFLT_INTERFACE(FilterStereo16BitFIRFilterMix)
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETSTEREOVOL16FIRFILTER
	SNDMIX_PROCESSSTEREOFILTER
	SNDMIX_STORESTEREOVOL
END_MIX_STFLT_INTERFACE()


//end rewbs.resamplerConf
// -! BEHAVIOUR_CHANGE#0025

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

// -> CODE#0025
// -> DESC="enable polyphase resampling on stereo samples"
// rewbs.resamplerConf
//Cubic stereo + ramp + reso filter
BEGIN_RAMPMIX_STFLT_INTERFACE(FilterStereo8BitHQRampMix)
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETSTEREOVOL8HQSRC
	SNDMIX_PROCESSSTEREOFILTER
	SNDMIX_RAMPSTEREOVOL
END_RAMPMIX_STFLT_INTERFACE()

BEGIN_RAMPMIX_STFLT_INTERFACE(FilterStereo16BitHQRampMix)
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETSTEREOVOL16HQSRC
	SNDMIX_PROCESSSTEREOFILTER
	SNDMIX_RAMPSTEREOVOL
END_RAMPMIX_STFLT_INTERFACE()

//Polyphase stereo + ramp + reso filter
BEGIN_RAMPMIX_STFLT_INTERFACE(FilterStereo8BitKaiserRampMix)
	SNDMIX_INITSINCTABLE
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETSTEREOVOL8KAISER
	SNDMIX_PROCESSSTEREOFILTER
	SNDMIX_RAMPSTEREOVOL
END_RAMPMIX_STFLT_INTERFACE()

BEGIN_RAMPMIX_STFLT_INTERFACE(FilterStereo16BitKaiserRampMix)
	SNDMIX_INITSINCTABLE
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETSTEREOVOL16KAISER
	SNDMIX_PROCESSSTEREOFILTER
	SNDMIX_RAMPSTEREOVOL
END_RAMPMIX_STFLT_INTERFACE()

//FIR filter stereo + ramp + reso filter
BEGIN_RAMPMIX_STFLT_INTERFACE(FilterStereo8BitFIRFilterRampMix)
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETSTEREOVOL8FIRFILTER
	SNDMIX_PROCESSSTEREOFILTER
	SNDMIX_RAMPSTEREOVOL
END_RAMPMIX_STFLT_INTERFACE()

BEGIN_RAMPMIX_STFLT_INTERFACE(FilterStereo16BitFIRFilterRampMix)
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETSTEREOVOL16FIRFILTER
	SNDMIX_PROCESSSTEREOFILTER
	SNDMIX_RAMPSTEREOVOL
END_RAMPMIX_STFLT_INTERFACE()

// end rewbs.resamplerConf
// -! BEHAVIOUR_CHANGE#0025

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
// -> CODE#0025
// -> DESC="enable polyphase resampling on stereo samples"
#define FilterMono8BitHQMix				Mono8BitHQMix
#define FilterMono16BitHQMix			Mono16BitHQMix
#define FilterMono8BitKaiserMix			Mono8BitKaiserMix
#define FilterMono16BitKaiserMix		Mono16BitKaiserMix
#define FilterMono8BitHQRampMix			Mono8BitHQRampMix
#define FilterMono16BitHQRampMix		Mono16BitHQRampMix
#define FilterMono8BitKaiserRampMix		Mono8BitKaiserRampMix
#define FilterMono16BitKaiserRampMix	Mono16BitKaiserRampMix
#define FilterMono8BitFIRFilterRampMix		Mono8BitFIRFilterRampMix
#define FilterMono16BitFIRFilterRampMix	Mono16BitFIRFilterRampMix
// -! BEHAVIOUR_CHANGE#0025

// Stereo
#define FilterStereo8BitMix				Stereo8BitMix
#define FilterStereo16BitMix			Stereo16BitMix
#define FilterStereo8BitLinearMix		Stereo8BitLinearMix
#define FilterStereo16BitLinearMix		Stereo16BitLinearMix
#define FilterStereo8BitRampMix			Stereo8BitRampMix
#define FilterStereo16BitRampMix		Stereo16BitRampMix
#define FilterStereo8BitLinearRampMix	Stereo8BitLinearRampMix
#define FilterStereo16BitLinearRampMix	Stereo16BitLinearRampMix
// -> CODE#0025
// -> DESC="enable polyphase resampling on stereo samples"
#define FilterStereo8BitHQMix			Stereo8BitHQMix
#define FilterStereo16BitHQMix			Stereo16BitHQMix
#define FilterStereo8BitKaiserMix		Stereo8BitKaiserMix
#define FilterStereo16BitKaiserMix		Stereo16BitKaiserMix
#define FilterStereo8BitHQRampMix		Stereo8BitHQRampMix
#define FilterStereo16BitHQRampMix		Stereo16BitHQRampMix
#define FilterStereo8BitKaiserRampMix	Stereo8BitKaiserRampMix
#define FilterStereo16BitKaiserRampMix	Stereo16BitKaiserRampMix
#define FilterStereo8BitFIRFilterRampMix	Stereo8BitFIRFilterRampMix
#define FilterStereo16BitFIRFilterRampMix	Stereo16BitFIRFilterRampMix
// -! BEHAVIOUR_CHANGE#0025
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
#define MIXNDX_HQSRC		0x20
#define MIXNDX_KAISERSRC	0x30
#define MIXNDX_FIRFILTERSRC	0x40 // rewbs.resamplerConf


//const LPMIXINTERFACE gpMixFunctionTable[4*16] =
const LPMIXINTERFACE gpMixFunctionTable[5*16] =    //rewbs.resamplerConf: increased to 5 to cope with FIR
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
	// HQ SRC
	Mono8BitHQMix,				Mono16BitHQMix,				Stereo8BitHQMix,		Stereo16BitHQMix,
	Mono8BitHQRampMix,			Mono16BitHQRampMix,			Stereo8BitHQRampMix,	Stereo16BitHQRampMix,
	// HQ SRC, Filter

// -> CODE#0025
// -> DESC="enable polyphase resampling on stereo samples"
//	FilterMono8BitLinearMix,	FilterMono16BitLinearMix,	FilterStereo8BitLinearMix,	FilterStereo16BitLinearMix,
//	FilterMono8BitLinearRampMix,FilterMono16BitLinearRampMix,FilterStereo8BitLinearRampMix,FilterStereo16BitLinearRampMix,
	FilterMono8BitHQMix,		FilterMono16BitHQMix,		FilterStereo8BitHQMix,		FilterStereo16BitHQMix,
	FilterMono8BitHQRampMix,	FilterMono16BitHQRampMix,	FilterStereo8BitHQRampMix,	FilterStereo16BitHQRampMix,
// -! BEHAVIOUR_CHANGE#0025

	// Kaiser SRC
// -> CODE#0025
// -> DESC="enable polyphase resampling on stereo samples"
//	Mono8BitKaiserMix,			Mono16BitKaiserMix,			Stereo8BitHQMix,		Stereo16BitHQMix,
//	Mono8BitKaiserRampMix,		Mono16BitKaiserRampMix,		Stereo8BitHQRampMix,	Stereo16BitHQRampMix,
	Mono8BitKaiserMix,			Mono16BitKaiserMix,			Stereo8BitKaiserMix,	Stereo16BitKaiserMix,
	Mono8BitKaiserRampMix,		Mono16BitKaiserRampMix,		Stereo8BitKaiserRampMix,Stereo16BitKaiserRampMix,
// -! BEHAVIOUR_CHANGE#0025

	// Kaiser SRC, Filter
//	FilterMono8BitLinearMix,	FilterMono16BitLinearMix,	FilterStereo8BitLinearMix,	FilterStereo16BitLinearMix,
//	FilterMono8BitLinearRampMix,FilterMono16BitLinearRampMix,FilterStereo8BitLinearRampMix,FilterStereo16BitLinearRampMix,
	FilterMono8BitKaiserMix,	FilterMono16BitKaiserMix,	FilterStereo8BitKaiserMix,	FilterStereo16BitKaiserMix,
	FilterMono8BitKaiserRampMix,FilterMono16BitKaiserRampMix,FilterStereo8BitKaiserRampMix,FilterStereo16BitKaiserRampMix,

	// FIR Filter SRC
	Mono8BitFIRFilterMix,			Mono16BitFIRFilterMix,			Stereo8BitFIRFilterMix,	Stereo16BitFIRFilterMix,
	Mono8BitFIRFilterRampMix,		Mono16BitFIRFilterRampMix,		Stereo8BitFIRFilterRampMix,Stereo16BitFIRFilterRampMix,

	// FIR Filter SRC, Filter
	FilterMono8BitFIRFilterMix,	FilterMono16BitFIRFilterMix,	FilterStereo8BitFIRFilterMix,	FilterStereo16BitFIRFilterMix,
	FilterMono8BitFIRFilterRampMix,FilterMono16BitFIRFilterRampMix,FilterStereo8BitFIRFilterRampMix,FilterStereo16BitFIRFilterRampMix,

// -! BEHAVIOUR_CHANGE#0025


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


#ifdef ENABLE_MMX
//const LPMIXINTERFACE gpMMXFunctionTable[4*16] =
const LPMIXINTERFACE gpMMXFunctionTable[5*16] =     //rewbs.resamplerConf: increased to 5 to cope with FIR
{
	// No SRC
	MMX_Mono8BitMix,			MMX_Mono16BitMix,			Stereo8BitMix,			Stereo16BitMix,
	Mono8BitRampMix,			Mono16BitRampMix,			Stereo8BitRampMix,		Stereo16BitRampMix,
	// No SRC, Filter
	MMX_FilterMono8BitMix,		MMX_FilterMono16BitMix,		FilterStereo8BitMix,	FilterStereo16BitMix,
	MMX_FilterMono8BitRampMix,	MMX_FilterMono16BitRampMix,	FilterStereo8BitRampMix,FilterStereo16BitRampMix,
	// Linear SRC
	MMX_Mono8BitLinearMix,		MMX_Mono16BitLinearMix,		Stereo8BitLinearMix,	Stereo16BitLinearMix,
	Mono8BitLinearRampMix,		Mono16BitLinearRampMix,		Stereo8BitLinearRampMix,Stereo16BitLinearRampMix,
	// Linear SRC, Filter
	MMX_FilterMono8BitLinearMix,MMX_FilterMono16BitLinearMix,FilterStereo8BitLinearMix,	FilterStereo16BitLinearMix,
	MMX_FilterMono8BitLinearRampMix,MMX_FilterMono16BitLinearRampMix,FilterStereo8BitLinearRampMix,FilterStereo16BitLinearRampMix,
	// HQ SRC
	MMX_Mono8BitHQMix,			MMX_Mono16BitHQMix,			Stereo8BitHQMix,		Stereo16BitHQMix,
	Mono8BitHQRampMix,			Mono16BitHQRampMix,			Stereo8BitHQRampMix,	Stereo16BitHQRampMix,
	// HQ SRC, Filter
	MMX_FilterMono8BitLinearMix,MMX_FilterMono16BitLinearMix,FilterStereo8BitLinearMix,	FilterStereo16BitLinearMix,
	MMX_FilterMono8BitLinearRampMix,MMX_FilterMono16BitLinearRampMix,FilterStereo8BitLinearRampMix,FilterStereo16BitLinearRampMix,

	// Kaiser SRC
// -> CODE#0025
// -> DESC="enable polyphase resampling on stereo samples"
//	MMX_Mono8BitKaiserMix,		MMX_Mono16BitKaiserMix,		Stereo8BitHQMix,		Stereo16BitHQMix,
//	MMX_Mono8BitKaiserRampMix,	MMX_Mono16BitKaiserRampMix,	Stereo8BitHQRampMix,	Stereo16BitHQRampMix,
	MMX_Mono8BitKaiserMix,		MMX_Mono16BitKaiserMix,		Stereo8BitKaiserMix,	Stereo16BitKaiserMix,
	MMX_Mono8BitKaiserRampMix,	MMX_Mono16BitKaiserRampMix,	Stereo8BitKaiserRampMix,Stereo16BitKaiserRampMix,

	// Kaiser SRC, Filter - no ASM routines written for this yet.
	FilterMono8BitKaiserMix, FilterMono16BitKaiserMix,FilterStereo8BitKaiserMix,	FilterStereo16BitKaiserMix,
	FilterMono8BitKaiserRampMix, FilterMono16BitKaiserRampMix,FilterStereo8BitKaiserRampMix,FilterStereo16BitKaiserRampMix,

	// FIRFilter SRC - no ASM routines written for this yet.
	Mono8BitFIRFilterMix,		Mono16BitFIRFilterMix,		Stereo8BitFIRFilterMix,	Stereo16BitFIRFilterMix,
	Mono8BitFIRFilterRampMix,	Mono16BitFIRFilterRampMix,	Stereo8BitFIRFilterRampMix,Stereo16BitFIRFilterRampMix,

	// FIRFilter SRC, Filter - no ASM routines written for this yet.
	FilterMono8BitFIRFilterMix, FilterMono16BitFIRFilterMix,FilterStereo8BitFIRFilterMix,	FilterStereo16BitFIRFilterMix,
	FilterMono8BitFIRFilterRampMix, FilterMono16BitFIRFilterRampMix,FilterStereo8BitFIRFilterRampMix,FilterStereo16BitFIRFilterRampMix,


// -! BEHAVIOUR_CHANGE#0025
};
#endif


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
#ifdef _DEBUG
	{
		LONG nDeltaHi = (nInc>>16) * (nSmpCount - 1);
		LONG nDeltaLo = (nInc&0xffff) * (nSmpCount - 1);
		LONG nPosDest = nPos + nDeltaHi + ((nPosLo + nDeltaLo)>>16);
		if ((nPosDest < 0) || (nPosDest > (LONG)pChn->nLength))
		{
			Log("Incorrect delta:\n");
			Log("nSmpCount=%d: nPos=%5d.x%04X Len=%5d Inc=%2d.x%04X\n",
				nSmpCount, nPos, nPosLo, pChn->nLength, pChn->nInc>>16, pChn->nInc&0xffff);
			return 0;
		}
	}
#endif
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
	BOOL bSurround;
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
		//rewbs.resamplerConf
		nFlags |= GetResamplingFlag(pChannel);
		//end rewbs.resamplerConf
	#ifdef ENABLE_MMX
		if ((gdwSysInfo & SYSMIX_ENABLEMMX) && (gdwSoundSetup & SNDMIX_ENABLEMMX))
		{
			pMixFuncTable = gpMMXFunctionTable;
		} else
	#endif
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
		if ((pChannel->dwFlags & CHN_SURROUND) && (gnChannels > 2)) pbuffer = MixRearBuffer;
		if (pChannel->dwFlags & CHN_NOREVERB) pbuffer = MixSoundBuffer;

	#ifdef ENABLE_MMX
		if ((pChannel->dwFlags & CHN_REVERB) && (gdwSysInfo & SYSMIX_ENABLEMMX))
			pbuffer = MixReverbBuffer;
	#endif

		//Look for plugins associated with this implicit tracker channel.
		UINT nMixPlugin = GetBestPlugin(ChnMix[nChn], PRIORITISE_INSTRUMENT, RESPECT_MUTES);
		
		//rewbs.instroVSTi
/*		UINT nMixPlugin=0;
		if (pChannel->pHeader && pChannel->pInstrument) {	// first try intrument VST
			if (!(pChannel->pInstrument->uFlags & ENV_MUTE))
				nMixPlugin = pChannel->pHeader->nMixPlug;
		}
		if (!nMixPlugin && (nMasterCh > 0) && (nMasterCh <= m_nChannels)) { 	// Then try Channel VST
			if(!(pChannel->dwFlags & CHN_NOFX)) 
				nMixPlugin = ChnSettings[nMasterCh-1].nMixPlugin;
		}
*/

		//end rewbs.instroVSTi		
		if ((nMixPlugin > 0) && (nMixPlugin <= MAX_MIXPLUGINS))
		{
			PSNDMIXPLUGINSTATE pPlugin = m_MixPlugins[nMixPlugin-1].pMixState;
			if ((pPlugin) && (pPlugin->pMixBuffer))
			{
				pbuffer = pPlugin->pMixBuffer;
				pOfsR = &pPlugin->nVolDecayR;
				pOfsL = &pPlugin->nVolDecayL;
				if (!(pPlugin->dwFlags & MIXPLUG_MIXREADY))
				{
					X86_StereoFill(pbuffer, count, pOfsR, pOfsL);
					pPlugin->dwFlags |= MIXPLUG_MIXREADY;
				}
			}
		}
		if (pbuffer == MixReverbBuffer)
		{
			if (!gnReverbSend)
			{
				X86_StereoFill(MixReverbBuffer, count, &gnRvbROfsVol, &gnRvbLOfsVol);
			}
			gnReverbSend += count;
			pOfsR = &gnRvbROfsVol;
			pOfsL = &gnRvbLOfsVol;
		}
	#ifndef FASTSOUNDLIB
		bSurround = (pbuffer == MixRearBuffer);
	#endif
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
#ifdef ENABLE_MMX
	if ((gdwSysInfo & SYSMIX_ENABLEMMX) && (gdwSoundSetup & SNDMIX_ENABLEMMX))
	{
		MMX_EndMix();
	}
#endif
	return nchused;
}

UINT CSoundFile::GetResamplingFlag(const MODCHANNEL *pChannel)
//------------------------------------------------------------
{
	if (pChannel->pHeader) {
		switch (pChannel->pHeader->nResampling) {
			case SRCMODE_NEAREST:	return 0;
			case SRCMODE_LINEAR:	return MIXNDX_LINEARSRC;
			case SRCMODE_SPLINE:	return MIXNDX_HQSRC;
			case SRCMODE_POLYPHASE: return MIXNDX_KAISERSRC;
			case SRCMODE_FIRFILTER: return MIXNDX_FIRFILTERSRC;
//			default: ;
		}
	}
	
	//didn't manage to get flag from instrument header, use channel flags.
	if (pChannel->dwFlags & CHN_HQSRC)	{
		if (gdwSoundSetup & SNDMIX_SPLINESRCMODE)		return MIXNDX_HQSRC;
		if (gdwSoundSetup & SNDMIX_POLYPHASESRCMODE)	return MIXNDX_KAISERSRC;
		if (gdwSoundSetup & SNDMIX_FIRFILTERSRCMODE)	return MIXNDX_FIRFILTERSRC;				
	} else if (!(pChannel->dwFlags & CHN_NOIDO)) {
		return MIXNDX_LINEARSRC;
	}
	
	return 0;
}


extern int gbInitPlugins;

VOID CSoundFile::ProcessPlugins(UINT nCount)
//------------------------------------------
{
	// Setup float inputs
	for (UINT iPlug=0; iPlug<MAX_MIXPLUGINS; iPlug++)
	{
		PSNDMIXPLUGIN pPlugin = &m_MixPlugins[iPlug];
		if ((pPlugin->pMixPlugin) && (pPlugin->pMixState)
		 && (pPlugin->pMixState->pMixBuffer)
		 && (pPlugin->pMixState->pOutBufferL)
		 && (pPlugin->pMixState->pOutBufferR))
		{
			PSNDMIXPLUGINSTATE pState = pPlugin->pMixState;
			// Init plugins ?
			/*if (gbInitPlugins)
			{   //ToDo: do this in resume.
				pPlugin->pMixPlugin->Init(gdwMixingFreq, (gbInitPlugins & 2) ? TRUE : FALSE);
			}*/

			//We should only ever reach this point if the song is playing.
			if (!pPlugin->pMixPlugin->IsSongPlaying()) {
				//Plugin doesn't know it is in a song that is playing;
				//we must have added it during playback. Initialise it!
				pPlugin->pMixPlugin->NotifySongPlaying(true);
				pPlugin->pMixPlugin->Resume();
			}


			// Setup float input
			if (pState->dwFlags & MIXPLUG_MIXREADY)
			{
				StereoMixToFloat(pState->pMixBuffer, pState->pOutBufferL, pState->pOutBufferR, nCount);
			} else
			if (pState->nVolDecayR|pState->nVolDecayL)
			{
				X86_StereoFill(pState->pMixBuffer, nCount, &pState->nVolDecayR, &pState->nVolDecayL);
				StereoMixToFloat(pState->pMixBuffer, pState->pOutBufferL, pState->pOutBufferR, nCount);
			} else
			{
				memset(pState->pOutBufferL, 0, nCount*sizeof(FLOAT));
				memset(pState->pOutBufferR, 0, nCount*sizeof(FLOAT));
			}
			pState->dwFlags &= ~MIXPLUG_MIXREADY;
		}
	}
	// Convert mix buffer
	StereoMixToFloat(MixSoundBuffer, MixFloatBuffer, MixFloatBuffer+MIXBUFFERSIZE, nCount);
	FLOAT *pMixL = MixFloatBuffer;
	FLOAT *pMixR = MixFloatBuffer + MIXBUFFERSIZE;

	// Process Plugins
	for (UINT iDoPlug=0; iDoPlug<MAX_MIXPLUGINS; iDoPlug++)
	{
		PSNDMIXPLUGIN pPlugin = &m_MixPlugins[iDoPlug];
		if ((pPlugin->pMixPlugin) && (pPlugin->pMixState)
		 && (pPlugin->pMixState->pMixBuffer)
		 && (pPlugin->pMixState->pOutBufferL)
		 && (pPlugin->pMixState->pOutBufferR))
		{
			BOOL bMasterMix = FALSE;
			if (pMixL == pPlugin->pMixState->pOutBufferL)
			{
				bMasterMix = TRUE;
				pMixL = MixFloatBuffer;
				pMixR = MixFloatBuffer + MIXBUFFERSIZE;
			}
			IMixPlugin *pObject = pPlugin->pMixPlugin;
			PSNDMIXPLUGINSTATE pState = pPlugin->pMixState;
			FLOAT *pOutL = pMixL;
			FLOAT *pOutR = pMixR;

			if (pPlugin->Info.dwOutputRouting & 0x80)
			{
				UINT nOutput = pPlugin->Info.dwOutputRouting & 0x7f;
				if ((nOutput > iDoPlug) && (nOutput < MAX_MIXPLUGINS)
				 && (m_MixPlugins[nOutput].pMixState))
				{
					PSNDMIXPLUGINSTATE pOutState = m_MixPlugins[nOutput].pMixState;

					if( (pOutState->pOutBufferL) && (pOutState->pOutBufferR) )
					{
						pOutL = pOutState->pOutBufferL;
						pOutR = pOutState->pOutBufferR;
					}
				}
			}

			if (pPlugin->Info.dwInputRouting & MIXPLUG_INPUTF_MASTEREFFECT)
			{
				if (!bMasterMix)
				{
					FLOAT *pInL = pState->pOutBufferL;
					FLOAT *pInR = pState->pOutBufferR;
					for (UINT i=0; i<nCount; i++)
					{
						pInL[i] += pMixL[i];
						pInR[i] += pMixR[i];
						pMixL[i] = 0;
						pMixR[i] = 0;
					}
				}
				pMixL = pOutL;
				pMixR = pOutR;
			}

			if (pPlugin->Info.dwInputRouting & MIXPLUG_INPUTF_BYPASS)
			{
				const FLOAT * const pInL = pState->pOutBufferL;
				const FLOAT * const pInR = pState->pOutBufferR;
				for (UINT i=0; i<nCount; i++)
				{
					pOutL[i] += pInL[i];
					pOutR[i] += pInR[i];
				}
			} else
			{
				pObject->Process(pOutL, pOutR, nCount);
			}
		}
	}
	FloatToStereoMix(pMixL, pMixR, MixSoundBuffer, nCount);
	gbInitPlugins = 0;
}

//////////////////////////////////////////////////////////////////////////////////////////
//
// Float <-> Int conversion
//


float CSoundFile::m_nMaxSample = 0;

VOID CSoundFile::StereoMixToFloat(const int *pSrc, float *pOut1, float *pOut2, UINT nCount)
//-----------------------------------------------------------------------------------------
{

#ifdef ENABLE_MMX
	if (gdwSoundSetup & SNDMIX_ENABLEMMX)
	{
		if (gdwSysInfo & SYSMIX_SSE)
		{
#ifdef ENABLE_SSE
		SSE_StereoMixToFloat(pSrc, pOut1, pOut2, nCount, m_pConfig->getIntToFloat());
#endif
			return;
		}
		if (gdwSysInfo & SYSMIX_3DNOW)
		{
#ifdef ENABLE_AMD
		AMD_StereoMixToFloat(pSrc, pOut1, pOut2, nCount, m_pConfig->getIntToFloat());
#endif
			return;
		}
	}
#endif

 	X86_StereoMixToFloat(pSrc, pOut1, pOut2, nCount, m_pConfig->getIntToFloat());

}


VOID CSoundFile::FloatToStereoMix(const float *pIn1, const float *pIn2, int *pOut, UINT nCount)
//---------------------------------------------------------------------------------------------
{
	if (gdwSoundSetup & SNDMIX_ENABLEMMX)
	{
		if (gdwSysInfo & SYSMIX_3DNOW)
		{
#ifdef ENABLE_AMDNOW
			AMD_FloatToStereoMix(pIn1, pIn2, pOut, nCount, m_pConfig->getFloatToInt());
#endif
			return;
		}
	}
	X86_FloatToStereoMix(pIn1, pIn2, pOut, nCount, m_pConfig->getFloatToInt());
}


VOID CSoundFile::MonoMixToFloat(const int *pSrc, float *pOut, UINT nCount)
//------------------------------------------------------------------------
{
	if (gdwSoundSetup & SNDMIX_ENABLEMMX)
	{
		if (gdwSysInfo & SYSMIX_SSE)
		{
#ifdef ENABLE_SSE
 		SSE_MonoMixToFloat(pSrc, pOut, nCount, m_pConfig->getIntToFloat());
#endif
			return;
		}
		if (gdwSysInfo & SYSMIX_3DNOW)
		{
#ifdef ENABLE_AMDNOW
			AMD_MonoMixToFloat(pSrc, pOut, nCount, m_pConfig->getIntToFloat());
#endif
			return;
		}
	}
	X86_MonoMixToFloat(pSrc, pOut, nCount, m_pConfig->getIntToFloat());	

}


VOID CSoundFile::FloatToMonoMix(const float *pIn, int *pOut, UINT nCount)
//-----------------------------------------------------------------------
{
	if (gdwSoundSetup & SNDMIX_ENABLEMMX)
	{
		if (gdwSysInfo & SYSMIX_3DNOW)
		{
#ifdef ENABLE_AMDNOW
			AMD_FloatToMonoMix(pIn, pOut, nCount, m_pConfig->getFloatToInt());
#endif
			return;
		}
	}
	X86_FloatToMonoMix(pIn, pOut, nCount, m_pConfig->getFloatToInt());
}


#pragma warning (disable:4100)

// Clip and convert to 8 bit
DWORD MPPASMCALL X86_Convert32To8(LPVOID lp16, int *pBuffer, DWORD lSampleCount)
//------------------------------------------------------------------------------
{
	DWORD result;
	_asm {
	mov ebx, lp16			// ebx = 8-bit buffer
	mov edx, pBuffer		// edx = pBuffer
	mov edi, lSampleCount	// edi = lSampleCount
cliploop:
	mov eax, dword ptr [edx]
	inc ebx
	add eax, (1<<(23-MIXING_ATTENUATION))
	add edx, 4
	cmp eax, MIXING_CLIPMIN
	jl cliplow
	cmp eax, MIXING_CLIPMAX
	jg cliphigh
cliprecover:
	sar eax, (24-MIXING_ATTENUATION)
	xor eax, 0x80
	dec edi
	mov byte ptr [ebx-1], al
	jnz cliploop
	jmp done
cliplow:
	mov eax, MIXING_CLIPMIN
	jmp cliprecover
cliphigh:
	mov eax, MIXING_CLIPMAX
	jmp cliprecover
done:
	mov eax, lSampleCount
	mov result, eax
	}
	return result;
}


// Clip and convert to 16 bit
DWORD MPPASMCALL X86_Convert32To16(LPVOID lp16, int *pBuffer, DWORD lSampleCount)
//-------------------------------------------------------------------------------
{
	DWORD result;
	_asm {
	mov ebx, lp16				// ebx = 16-bit buffer
	mov edx, pBuffer			// edx = pBuffer
	mov edi, lSampleCount		// edi = lSampleCount
cliploop:
	mov eax, dword ptr [edx]
	add ebx, 2
	add eax, (1<<(15-MIXING_ATTENUATION))
	add edx, 4
	cmp eax, MIXING_CLIPMIN
	jl cliplow
	cmp eax, MIXING_CLIPMAX
	jg cliphigh
cliprecover:
	sar eax, 16-MIXING_ATTENUATION
	dec edi
	mov word ptr [ebx-2], ax
	jnz cliploop
	jmp done
cliplow:
	mov eax, MIXING_CLIPMIN
	jmp cliprecover
cliphigh:
	mov eax, MIXING_CLIPMAX
	jmp cliprecover
done:
	mov eax, lSampleCount
	add eax, eax
	mov result, eax
	}
	return result;
}


// Clip and convert to 24 bit
DWORD MPPASMCALL X86_Convert32To24(LPVOID lp16, int *pBuffer, DWORD lSampleCount)
//-------------------------------------------------------------------------------
{
	DWORD result;
	_asm {
	mov ebx, lp16			// ebx = 8-bit buffer
	mov edx, pBuffer		// edx = pBuffer
	mov edi, lSampleCount	// edi = lSampleCount
cliploop:
	mov eax, dword ptr [edx]
	add ebx, 3
	//add eax, (7-MIXING_ATTENUATION)
	add eax, (1<<(7-MIXING_ATTENUATION))  //ericus' 24bit fix
	add edx, 4
	cmp eax, MIXING_CLIPMIN
	jl cliplow
	cmp eax, MIXING_CLIPMAX
	jg cliphigh
cliprecover:
	sar eax, (8-MIXING_ATTENUATION)
	mov word ptr [ebx-3], ax
	shr eax, 16
	dec edi
	mov byte ptr [ebx-1], al
	jnz cliploop
	jmp done
cliplow:
	mov eax, MIXING_CLIPMIN
	jmp cliprecover
cliphigh:
	mov eax, MIXING_CLIPMAX
	jmp cliprecover
done:
	mov eax, lSampleCount
	lea eax, [eax*2+eax]
	mov result, eax
	}
	return result;
}


// Clip and convert to 32 bit
DWORD MPPASMCALL X86_Convert32To32(LPVOID lp16, int *pBuffer, DWORD lSampleCount)
//-------------------------------------------------------------------------------
{
	DWORD result;
	_asm {
	mov ebx, lp16			// ebx = 32-bit buffer
	mov edx, pBuffer		// edx = pBuffer
	mov edi, lSampleCount	// edi = lSampleCount
cliploop:
	mov eax, dword ptr [edx]
	add ebx, 4
	add edx, 4
	cmp eax, MIXING_CLIPMIN
	jl cliplow
	cmp eax, MIXING_CLIPMAX
	jg cliphigh
cliprecover:
	shl eax, MIXING_ATTENUATION
	dec edi
	mov dword ptr [ebx-4], eax
	jnz cliploop
	jmp done
cliplow:
	mov eax, MIXING_CLIPMIN
	jmp cliprecover
cliphigh:
	mov eax, MIXING_CLIPMAX
	jmp cliprecover
done:
	mov eax, lSampleCount
	lea eax, [eax*4]
	mov result, eax
	}
	return result;
}


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


//////////////////////////////////////////////////////////////////////////
// Noise Shaping (Dither)

#ifndef FASTSOUNDLIB

#pragma warning(disable:4731) // ebp modified

void MPPASMCALL X86_Dither(int *pBuffer, UINT nSamples, UINT nBits)
//-----------------------------------------------------------------
{
	static int gDitherA, gDitherB;

	__asm {
	mov esi, pBuffer	// esi = pBuffer+i
	mov eax, nSamples	// ebp = i
	mov ecx, nBits		// ecx = number of bits of noise
	mov edi, gDitherA	// Noise generation
	mov ebx, gDitherB
	add ecx, MIXING_ATTENUATION+1
	push ebp
	mov ebp, eax
noiseloop:
	rol edi, 1
	mov eax, dword ptr [esi]
	xor edi, 0x10204080
	add esi, 4
	lea edi, [ebx*4+edi+0x78649E7D]
	mov edx, edi
	rol edx, 16
	lea edx, [edx*4+edx]
	add ebx, edx
	mov edx, ebx
	sar edx, cl
	add eax, edx
/*
	int a = 0, b = 0;
	for (UINT i=0; i<len; i++)
	{
		a = (a << 1) | (((DWORD)a) >> (BYTE)31);
		a ^= 0x10204080;
		a += 0x78649E7D + (b << 2);
		b += ((a << 16) | (a >> 16)) * 5;
		int c = a + b;
		p[i] = ((signed char)c ) >> 1;
	}
*/
	dec ebp
	mov dword ptr [esi-4], eax
	jnz noiseloop
	pop ebp
	mov gDitherA, edi
	mov gDitherB, ebx
	}
}


void MPPASMCALL X86_InterleaveFrontRear(int *pFrontBuf, int *pRearBuf, DWORD nSamples)
//------------------------------------------------------------------------------------
{
	_asm {
	mov ecx, nSamples	// ecx = samplecount
	mov esi, pFrontBuf	// esi = front buffer
	mov edi, pRearBuf	// edi = rear buffer
	lea esi, [esi+ecx*4]	// esi = &front[N]
	lea edi, [edi+ecx*4]	// edi = &rear[N]
	lea ebx, [esi+ecx*4]	// ebx = &front[N*2]
	push ebp
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
	pop ebp
	}
}

#endif // FASTSOUNDLIB


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

#define OFSDECAYSHIFT	8
#define OFSDECAYMASK	0xFF


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


//////////////////////////////////////////////////////////////////////////////////
// Automatic Gain Control

#ifndef NO_AGC

// Limiter
#define MIXING_LIMITMAX		(0x08100000)
#define MIXING_LIMITMIN		(-MIXING_LIMITMAX)

UINT MPPASMCALL X86_AGC(int *pBuffer, UINT nSamples, UINT nAGC)
//-------------------------------------------------------------
{
	UINT result;
	__asm {
	mov esi, pBuffer	// esi = pBuffer+i
	mov ecx, nSamples	// ecx = i
	mov edi, nAGC		// edi = AGC (0..256)
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
	jmp done
agcupdate:
	dec edi
	jmp agcrecover
done:
	mov result, edi
	}
	return result;
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
	if ((agc >= gnAGC) && (gnAGC < AGC_UNITY))
	{
		gAGCRecoverCount += count;
		UINT agctimeout = gdwMixingFreq >> (AGC_PRECISION-8);
		if (gnChannels < 2) agctimeout >>= 1;
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




