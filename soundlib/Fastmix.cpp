/*
 * Fastmix.cpp
 * -----------
 * Purpose: Mixer core for rendering samples, mixing plugins, etc...
 * Notes  : If this is Fastmix.cpp, where is Slowmix.cpp? :)
 *          This code is ugly like hell and you are probably not going to understand it
 *          unless you have worked with OpenMPT for a long time - at least I didn't. :)
 *          I guess that a lot of this could be refactored using inline functions.
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Sndfile.h"

#include "MixerLoops.h"
#include "Resampler.h"
#include "WindowedFIR.h"


// 4x256 taps polyphase FIR resampling filter
#define gFastSinc CResampler::FastSincTable


/////////////////////////////////////////////////////
// Mixing Macros

#define SNDMIX_BEGINSAMPLELOOP8\
	register ModChannel * const pChn = pChannel;\
	nPos = pChn->nPosLo;\
	const signed char *p = (signed char *)(pChn->pCurrentSample)+pChn->nPos;\
	if (pChn->dwFlags[CHN_STEREO]) p += pChn->nPos;\
	int *pvol = pbuffer;\
	do {

#define SNDMIX_BEGINSAMPLELOOP16\
	register ModChannel * const pChn = pChannel;\
	nPos = pChn->nPosLo;\
	const signed short *p = (signed short *)(pChn->pCurrentSample)+pChn->nPos;\
	if (pChn->dwFlags[CHN_STEREO]) p += pChn->nPos;\
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
	const SINC_TYPE *poslo = sinc + ((nPos >> (16-SINC_PHASES_BITS)) & SINC_MASK) * SINC_WIDTH;\
	int vol = ((poslo[0]*p[poshi-3] + poslo[1]*p[poshi-2]\
		 + poslo[2]*p[poshi-1] + poslo[3]*p[poshi]\
		 + poslo[4]*p[poshi+1] + poslo[5]*p[poshi+2]\
		 + poslo[6]*p[poshi+3] + poslo[7]*p[poshi+4]) >> (SINC_QUANTSHIFT-8));\

#define SNDMIX_GETMONOVOL16KAISER\
	int poshi = nPos >> 16;\
	const SINC_TYPE *poslo = sinc + ((nPos >> (16-SINC_PHASES_BITS)) & SINC_MASK) * SINC_WIDTH;\
	int vol = ((poslo[0]*p[poshi-3] + poslo[1]*p[poshi-2]\
		 + poslo[2]*p[poshi-1] + poslo[3]*p[poshi]\
		 + poslo[4]*p[poshi+1] + poslo[5]*p[poshi+2]\
		 + poslo[6]*p[poshi+3] + poslo[7]*p[poshi+4]) >> SINC_QUANTSHIFT);\
// rewbs.resamplerConf
#define SNDMIX_GETMONOVOL8FIRFILTER \
	int poshi  = nPos >> 16;\
	int poslo  = (nPos & 0xFFFF);\
	int firidx = ((poslo+WFIR_FRACHALVE)>>WFIR_FRACSHIFT) & WFIR_FRACMASK; \
	int vol    = (WFIR_lut[firidx+0]*(int)p[poshi+1-4]);	\
        vol   += (WFIR_lut[firidx+1]*(int)p[poshi+2-4]);	\
        vol   += (WFIR_lut[firidx+2]*(int)p[poshi+3-4]);	\
        vol   += (WFIR_lut[firidx+3]*(int)p[poshi+4-4]);	\
        vol   += (WFIR_lut[firidx+4]*(int)p[poshi+5-4]);	\
        vol   += (WFIR_lut[firidx+5]*(int)p[poshi+6-4]);	\
        vol   += (WFIR_lut[firidx+6]*(int)p[poshi+7-4]);	\
        vol   += (WFIR_lut[firidx+7]*(int)p[poshi+8-4]);	\
        vol  >>= WFIR_8SHIFT;

#define SNDMIX_GETMONOVOL16FIRFILTER \
    int poshi  = nPos >> 16;\
    int poslo  = (nPos & 0xFFFF);\
    int firidx = ((poslo+WFIR_FRACHALVE)>>WFIR_FRACSHIFT) & WFIR_FRACMASK; \
    int vol1   = (WFIR_lut[firidx+0]*(int)p[poshi+1-4]);	\
        vol1  += (WFIR_lut[firidx+1]*(int)p[poshi+2-4]);	\
        vol1  += (WFIR_lut[firidx+2]*(int)p[poshi+3-4]);	\
        vol1  += (WFIR_lut[firidx+3]*(int)p[poshi+4-4]);	\
    int vol2   = (WFIR_lut[firidx+4]*(int)p[poshi+5-4]);	\
		vol2  += (WFIR_lut[firidx+5]*(int)p[poshi+6-4]);	\
		vol2  += (WFIR_lut[firidx+6]*(int)p[poshi+7-4]);	\
		vol2  += (WFIR_lut[firidx+7]*(int)p[poshi+8-4]);	\
    int vol    = ((vol1>>1)+(vol2>>1)) >> (WFIR_16BITSHIFT-1);


// end rewbs.resamplerConf
#define SNDMIX_INITSINCTABLE\
	const SINC_TYPE * const sinc = (((pChannel->nInc > 0x13000) || (pChannel->nInc < -0x13000)) ?\
		(((pChannel->nInc > 0x18000) || (pChannel->nInc < -0x18000)) ? pResampler->gDownsample2x : pResampler->gDownsample13x) : pResampler->gKaiserSinc);

#define SNDMIX_INITFIRTABLE\
	const signed short * const WFIR_lut = pResampler->m_WindowedFIR.lut;


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
	const SINC_TYPE *poslo = sinc + ((nPos >> (16-SINC_PHASES_BITS)) & SINC_MASK) * SINC_WIDTH;\
	int vol_l = ((poslo[0]*p[poshi*2-6] + poslo[1]*p[poshi*2-4]\
		 + poslo[2]*p[poshi*2-2] + poslo[3]*p[poshi*2]\
		 + poslo[4]*p[poshi*2+2] + poslo[5]*p[poshi*2+4]\
		 + poslo[6]*p[poshi*2+6] + poslo[7]*p[poshi*2+8]) >> (SINC_QUANTSHIFT-8));\
	int vol_r = ((poslo[0]*p[poshi*2-5] + poslo[1]*p[poshi*2-3]\
		 + poslo[2]*p[poshi*2-1] + poslo[3]*p[poshi*2+1]\
		 + poslo[4]*p[poshi*2+3] + poslo[5]*p[poshi*2+5]\
		 + poslo[6]*p[poshi*2+7] + poslo[7]*p[poshi*2+9]) >> (SINC_QUANTSHIFT-8));\

#define SNDMIX_GETSTEREOVOL16KAISER\
	int poshi = nPos >> 16;\
	const SINC_TYPE *poslo = sinc + ((nPos >> (16-SINC_PHASES_BITS)) & SINC_MASK) * SINC_WIDTH;\
	int vol_l = ((poslo[0]*p[poshi*2-6] + poslo[1]*p[poshi*2-4]\
		 + poslo[2]*p[poshi*2-2] + poslo[3]*p[poshi*2]\
		 + poslo[4]*p[poshi*2+2] + poslo[5]*p[poshi*2+4]\
		 + poslo[6]*p[poshi*2+6] + poslo[7]*p[poshi*2+8]) >> SINC_QUANTSHIFT);\
	int vol_r = ((poslo[0]*p[poshi*2-5] + poslo[1]*p[poshi*2-3]\
		 + poslo[2]*p[poshi*2-1] + poslo[3]*p[poshi*2+1]\
		 + poslo[4]*p[poshi*2+3] + poslo[5]*p[poshi*2+5]\
		 + poslo[6]*p[poshi*2+7] + poslo[7]*p[poshi*2+9]) >> SINC_QUANTSHIFT);\
// rewbs.resamplerConf
// fir interpolation
#define SNDMIX_GETSTEREOVOL8FIRFILTER \
    int poshi   = nPos >> 16;\
    int poslo   = (nPos & 0xFFFF);\
    int firidx  = ((poslo+WFIR_FRACHALVE)>>WFIR_FRACSHIFT) & WFIR_FRACMASK; \
    int vol_l   = (WFIR_lut[firidx+0]*(int)p[(poshi+1-4)*2  ]);   \
		vol_l  += (WFIR_lut[firidx+1]*(int)p[(poshi+2-4)*2  ]);   \
		vol_l  += (WFIR_lut[firidx+2]*(int)p[(poshi+3-4)*2  ]);   \
        vol_l  += (WFIR_lut[firidx+3]*(int)p[(poshi+4-4)*2  ]);   \
        vol_l  += (WFIR_lut[firidx+4]*(int)p[(poshi+5-4)*2  ]);   \
		vol_l  += (WFIR_lut[firidx+5]*(int)p[(poshi+6-4)*2  ]);   \
		vol_l  += (WFIR_lut[firidx+6]*(int)p[(poshi+7-4)*2  ]);   \
        vol_l  += (WFIR_lut[firidx+7]*(int)p[(poshi+8-4)*2  ]);   \
		vol_l >>= WFIR_8SHIFT; \
    int vol_r   = (WFIR_lut[firidx+0]*(int)p[(poshi+1-4)*2+1]);   \
		vol_r  += (WFIR_lut[firidx+1]*(int)p[(poshi+2-4)*2+1]);   \
		vol_r  += (WFIR_lut[firidx+2]*(int)p[(poshi+3-4)*2+1]);   \
		vol_r  += (WFIR_lut[firidx+3]*(int)p[(poshi+4-4)*2+1]);   \
		vol_r  += (WFIR_lut[firidx+4]*(int)p[(poshi+5-4)*2+1]);   \
        vol_r  += (WFIR_lut[firidx+5]*(int)p[(poshi+6-4)*2+1]);   \
        vol_r  += (WFIR_lut[firidx+6]*(int)p[(poshi+7-4)*2+1]);   \
        vol_r  += (WFIR_lut[firidx+7]*(int)p[(poshi+8-4)*2+1]);   \
        vol_r >>= WFIR_8SHIFT;

#define SNDMIX_GETSTEREOVOL16FIRFILTER \
    int poshi   = nPos >> 16;\
    int poslo   = (nPos & 0xFFFF);\
    int firidx  = ((poslo+WFIR_FRACHALVE)>>WFIR_FRACSHIFT) & WFIR_FRACMASK; \
    int vol1_l  = (WFIR_lut[firidx+0]*(int)p[(poshi+1-4)*2  ]);   \
		vol1_l += (WFIR_lut[firidx+1]*(int)p[(poshi+2-4)*2  ]);   \
        vol1_l += (WFIR_lut[firidx+2]*(int)p[(poshi+3-4)*2  ]);   \
		vol1_l += (WFIR_lut[firidx+3]*(int)p[(poshi+4-4)*2  ]);   \
	int vol2_l  = (WFIR_lut[firidx+4]*(int)p[(poshi+5-4)*2  ]);   \
		vol2_l += (WFIR_lut[firidx+5]*(int)p[(poshi+6-4)*2  ]);   \
		vol2_l += (WFIR_lut[firidx+6]*(int)p[(poshi+7-4)*2  ]);   \
		vol2_l += (WFIR_lut[firidx+7]*(int)p[(poshi+8-4)*2  ]);   \
	int vol_l   = ((vol1_l>>1)+(vol2_l>>1)) >> (WFIR_16BITSHIFT-1); \
	int vol1_r  = (WFIR_lut[firidx+0]*(int)p[(poshi+1-4)*2+1]);   \
		vol1_r += (WFIR_lut[firidx+1]*(int)p[(poshi+2-4)*2+1]);   \
		vol1_r += (WFIR_lut[firidx+2]*(int)p[(poshi+3-4)*2+1]);   \
		vol1_r += (WFIR_lut[firidx+3]*(int)p[(poshi+4-4)*2+1]);   \
	int vol2_r  = (WFIR_lut[firidx+4]*(int)p[(poshi+5-4)*2+1]);   \
		vol2_r += (WFIR_lut[firidx+5]*(int)p[(poshi+6-4)*2+1]);   \
		vol2_r += (WFIR_lut[firidx+6]*(int)p[(poshi+7-4)*2+1]);   \
		vol2_r += (WFIR_lut[firidx+7]*(int)p[(poshi+8-4)*2+1]);   \
	int vol_r   = ((vol1_r>>1)+(vol2_r>>1)) >> (WFIR_16BITSHIFT-1);
//end rewbs.resamplerConf
// -! BEHAVIOUR_CHANGE#0025


/////////////////////////////////////////////////////////////////////////////

#define SNDMIX_STOREMONOVOL\
	pvol[0] += vol * pChn->leftVol;\
	pvol[1] += vol * pChn->rightVol;\
	pvol += 2;

#define SNDMIX_STORESTEREOVOL\
	pvol[0] += vol_l * pChn->leftVol;\
	pvol[1] += vol_r * pChn->rightVol;\
	pvol += 2;

#define SNDMIX_STOREFASTMONOVOL\
	int v = vol * pChn->leftVol;\
	pvol[0] += v;\
	pvol[1] += v;\
	pvol += 2;

#define SNDMIX_RAMPMONOVOL\
	rampLeftVol += pChn->leftRamp;\
	rampRightVol += pChn->rightRamp;\
	pvol[0] += vol * (rampLeftVol >> VOLUMERAMPPRECISION);\
	pvol[1] += vol * (rampRightVol >> VOLUMERAMPPRECISION);\
	pvol += 2;

#define SNDMIX_RAMPFASTMONOVOL\
	rampLeftVol += pChn->leftRamp;\
	int fastvol = vol * (rampLeftVol >> VOLUMERAMPPRECISION);\
	pvol[0] += fastvol;\
	pvol[1] += fastvol;\
	pvol += 2;

#define SNDMIX_RAMPSTEREOVOL\
	rampLeftVol += pChn->leftRamp;\
	rampRightVol += pChn->rightRamp;\
	pvol[0] += vol_l * (rampLeftVol >> VOLUMERAMPPRECISION);\
	pvol[1] += vol_r * (rampRightVol >> VOLUMERAMPPRECISION);\
	pvol += 2;


///////////////////////////////////////////////////
// Resonant Filters

// Filter values are clipped to double the input range (assuming input is 16-Bit, which it currently is)
#define ClipFilter(x) Clamp(x, 2.0f * (float)int16_min, 2.0f * (float)int16_max)

// Resonant filter for Mono samples
static forceinline void ProcessMonoFilter(int &vol, ModChannel *pChn)
//-------------------------------------------------------------------
{
	float fy1 = pChn->nFilter_Y1;
	float fy2 = pChn->nFilter_Y2;

	float fy = ((float)vol * pChn->nFilter_A0 + ClipFilter(fy1) * pChn->nFilter_B0 + ClipFilter(fy2) * pChn->nFilter_B1);
	fy2 = fy1;
	fy1 = fy - (float)(vol & pChn->nFilter_HP);
	vol = (int)fy;

	pChn->nFilter_Y1 = fy1;
	pChn->nFilter_Y2 = fy2;
}


// Resonant filter for Stereo samples
static forceinline void ProcessStereoFilter(int &vol_l, int &vol_r, ModChannel *pChn)
//-----------------------------------------------------------------------------------
{
	// Left channel
	float fy1 = pChn->nFilter_Y1;
	float fy2 = pChn->nFilter_Y2;

	float fy = ((float)vol_l * pChn->nFilter_A0 + ClipFilter(fy1) * pChn->nFilter_B0 + ClipFilter(fy2) * pChn->nFilter_B1);
	fy2 = fy1;
	fy1 = fy - (float)(vol_l & pChn->nFilter_HP);
	vol_l = (int)fy;

	pChn->nFilter_Y1 = fy1;
	pChn->nFilter_Y2 = fy2;

	// Right channel
	fy1 = pChn->nFilter_Y3;
	fy2 = pChn->nFilter_Y4;

	fy = ((float)vol_r * pChn->nFilter_A0 + ClipFilter(fy1) * pChn->nFilter_B0 + ClipFilter(fy2) * pChn->nFilter_B1);
	fy2 = fy1;
	fy1 = fy - (float)(vol_r & pChn->nFilter_HP);
	vol_r = (int)fy;

	pChn->nFilter_Y3 = fy1;
	pChn->nFilter_Y4 = fy2;
}


// Mono
#define SNDMIX_PROCESSFILTER \
	ProcessMonoFilter(vol, pChn);


// Stereo
#define SNDMIX_PROCESSSTEREOFILTER \
	ProcessStereoFilter(vol_l, vol_r, pChn);



//////////////////////////////////////////////////////////
// Interfaces

typedef void (* LPMIXINTERFACE)(ModChannel *, const CResampler *, int *, int *);

#define BEGIN_MIX_INTERFACE(func)\
	void func(ModChannel *pChannel, const CResampler *pResampler, int *pbuffer, int *pbufmax)\
	{\
		MPT_UNREFERENCED_PARAMETER(pResampler);\
		LONG nPos;

#define END_MIX_INTERFACE()\
		SNDMIX_ENDSAMPLELOOP\
	}

// Volume Ramps
#define BEGIN_RAMPMIX_INTERFACE(func)\
	BEGIN_MIX_INTERFACE(func)\
		LONG rampLeftVol = pChannel->rampLeftVol;\
		LONG rampRightVol = pChannel->rampRightVol;

#define END_RAMPMIX_INTERFACE()\
		SNDMIX_ENDSAMPLELOOP\
		pChannel->rampLeftVol = rampLeftVol;\
		pChannel->leftVol = rampLeftVol >> VOLUMERAMPPRECISION;\
		pChannel->rampRightVol = rampRightVol;\
		pChannel->rightVol = rampRightVol >> VOLUMERAMPPRECISION;\
	}

#define BEGIN_FASTRAMPMIX_INTERFACE(func)\
	BEGIN_MIX_INTERFACE(func)\
		LONG rampLeftVol = pChannel->rampLeftVol;

#define END_FASTRAMPMIX_INTERFACE()\
		SNDMIX_ENDSAMPLELOOP\
		pChannel->rampLeftVol = rampLeftVol;\
		pChannel->rampRightVol = rampLeftVol;\
		pChannel->leftVol = rampLeftVol >> VOLUMERAMPPRECISION;\
		pChannel->rightVol = pChannel->leftVol;\
	}


// Mono Resonant Filters
#define BEGIN_MIX_FLT_INTERFACE(func)\
	BEGIN_MIX_INTERFACE(func)


#define END_MIX_FLT_INTERFACE()\
	SNDMIX_ENDSAMPLELOOP\
	}

#define BEGIN_RAMPMIX_FLT_INTERFACE(func)\
	BEGIN_MIX_INTERFACE(func)\
		LONG rampLeftVol = pChannel->rampLeftVol;\
		LONG rampRightVol = pChannel->rampRightVol;

#define END_RAMPMIX_FLT_INTERFACE()\
		SNDMIX_ENDSAMPLELOOP\
		pChannel->rampLeftVol = rampLeftVol;\
		pChannel->leftVol = rampLeftVol >> VOLUMERAMPPRECISION;\
		pChannel->rampRightVol = rampRightVol;\
		pChannel->rightVol = rampRightVol >> VOLUMERAMPPRECISION;\
	}

// Stereo Resonant Filters
#define BEGIN_MIX_STFLT_INTERFACE(func)\
	BEGIN_MIX_INTERFACE(func)


#define END_MIX_STFLT_INTERFACE()\
	SNDMIX_ENDSAMPLELOOP\
	}

#define BEGIN_RAMPMIX_STFLT_INTERFACE(func)\
	BEGIN_MIX_INTERFACE(func)\
		LONG rampLeftVol = pChannel->rampLeftVol;\
		LONG rampRightVol = pChannel->rampRightVol;

#define END_RAMPMIX_STFLT_INTERFACE()\
		SNDMIX_ENDSAMPLELOOP\
		pChannel->rampLeftVol = rampLeftVol;\
		pChannel->leftVol = rampLeftVol >> VOLUMERAMPPRECISION;\
		pChannel->rampRightVol = rampRightVol;\
		pChannel->rightVol = rampRightVol >> VOLUMERAMPPRECISION;\
	}


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

//////////////////////////////////////////////////////
// 8-taps polyphase resampling filter

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

// -> BEHAVIOUR_CHANGE#0025
// -> DESC="enable polyphase resampling on stereo samples"
// rewbs.resamplerConf
//////////////////////////////////////////////////////
// FIR filter

// Normal
BEGIN_MIX_INTERFACE(Mono8BitFIRFilterMix)
	SNDMIX_INITFIRTABLE
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETMONOVOL8FIRFILTER
	SNDMIX_STOREMONOVOL
END_MIX_INTERFACE()

BEGIN_MIX_INTERFACE(Mono16BitFIRFilterMix)
	SNDMIX_INITFIRTABLE
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETMONOVOL16FIRFILTER
	SNDMIX_STOREMONOVOL
END_MIX_INTERFACE()

// Ramp
BEGIN_RAMPMIX_INTERFACE(Mono8BitFIRFilterRampMix)
	SNDMIX_INITFIRTABLE
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETMONOVOL8FIRFILTER
	SNDMIX_RAMPMONOVOL
END_RAMPMIX_INTERFACE()

BEGIN_RAMPMIX_INTERFACE(Mono16BitFIRFilterRampMix)
	SNDMIX_INITFIRTABLE
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETMONOVOL16FIRFILTER
	SNDMIX_RAMPMONOVOL
END_RAMPMIX_INTERFACE()

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
	SNDMIX_INITFIRTABLE
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETSTEREOVOL8FIRFILTER
	SNDMIX_STORESTEREOVOL
END_MIX_INTERFACE()

BEGIN_MIX_INTERFACE(Stereo16BitFIRFilterMix)
	SNDMIX_INITFIRTABLE
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETSTEREOVOL16FIRFILTER
	SNDMIX_STORESTEREOVOL
END_MIX_INTERFACE()

// Ramp
BEGIN_RAMPMIX_INTERFACE(Stereo8BitFIRFilterRampMix)
	SNDMIX_INITFIRTABLE
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETSTEREOVOL8FIRFILTER
	SNDMIX_RAMPSTEREOVOL
END_RAMPMIX_INTERFACE()

BEGIN_RAMPMIX_INTERFACE(Stereo16BitFIRFilterRampMix)
	SNDMIX_INITFIRTABLE
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
	SNDMIX_INITFIRTABLE
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETMONOVOL8FIRFILTER
	SNDMIX_PROCESSFILTER
	SNDMIX_STOREMONOVOL
END_MIX_FLT_INTERFACE()

BEGIN_MIX_FLT_INTERFACE(FilterMono16BitFIRFilterMix)
	SNDMIX_INITFIRTABLE
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
	SNDMIX_INITFIRTABLE
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETMONOVOL8FIRFILTER
	SNDMIX_PROCESSFILTER
	SNDMIX_RAMPMONOVOL
END_RAMPMIX_FLT_INTERFACE()

BEGIN_RAMPMIX_FLT_INTERFACE(FilterMono16BitFIRFilterRampMix)
	SNDMIX_INITFIRTABLE
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
	SNDMIX_INITFIRTABLE
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETSTEREOVOL8FIRFILTER
	SNDMIX_PROCESSSTEREOFILTER
	SNDMIX_STORESTEREOVOL
END_MIX_STFLT_INTERFACE()

BEGIN_MIX_STFLT_INTERFACE(FilterStereo16BitFIRFilterMix)
	SNDMIX_INITFIRTABLE
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
	SNDMIX_INITFIRTABLE
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETSTEREOVOL8FIRFILTER
	SNDMIX_PROCESSSTEREOFILTER
	SNDMIX_RAMPSTEREOVOL
END_RAMPMIX_STFLT_INTERFACE()

BEGIN_RAMPMIX_STFLT_INTERFACE(FilterStereo16BitFIRFilterRampMix)
	SNDMIX_INITFIRTABLE
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
#define FilterMono8BitFIRFilterRampMix	Mono8BitFIRFilterRampMix
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

static UINT ResamplingModeToMixFlags(uint8 resamplingMode)
//--------------------------------------------------------
{
	switch(resamplingMode)
	{
	case SRCMODE_NEAREST:   return 0;
	case SRCMODE_LINEAR:    return MIXNDX_LINEARSRC;
	case SRCMODE_SPLINE:    return MIXNDX_HQSRC;
	case SRCMODE_POLYPHASE: return MIXNDX_KAISERSRC;
	case SRCMODE_FIRFILTER: return MIXNDX_FIRFILTERSRC;
	}
	return 0;
}


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


/////////////////////////////////////////////////////////////////////////

static forceinline LONG GetSampleCount(ModChannel *pChn, LONG nSamples, bool bITBidiMode)
//---------------------------------------------------------------------------------------
{
	LONG nLoopStart = pChn->dwFlags[CHN_LOOP] ? pChn->nLoopStart : 0;
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
			if(pChn->dwFlags[CHN_PINGPONGLOOP])
			{
				pChn->dwFlags.reset(CHN_PINGPONGFLAG); // go forward
			} else
			{
				pChn->dwFlags.set(CHN_PINGPONGFLAG);
				pChn->nPos = pChn->nLength - 1;
				pChn->nInc = -nInc;
			}
			if(!pChn->dwFlags[CHN_LOOP] || pChn->nPos >= pChn->nLength)
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
		if(!pChn->dwFlags[CHN_LOOP]) return 0; // not looping -> stop this channel
		if(pChn->dwFlags[CHN_PINGPONGLOOP])
		{
			// Invert loop
			if (nInc > 0)
			{
				nInc = -nInc;
				pChn->nInc = nInc;
			}
			pChn->dwFlags.set(CHN_PINGPONGFLAG);
			// adjust loop position
			LONG nDeltaHi = (pChn->nPos - pChn->nLength);
			LONG nDeltaLo = 0x10000 - (pChn->nPosLo & 0xffff);
			pChn->nPos = pChn->nLength - nDeltaHi - (nDeltaLo>>16);
			pChn->nPosLo = nDeltaLo & 0xffff;
			// Impulse Tracker's software mixer would put a -2 (instead of -1) in the following line (doesn't happen on a GUS)
			if ((pChn->nPos <= pChn->nLoopStart) || (pChn->nPos >= pChn->nLength)) pChn->nPos = pChn->nLength - (bITBidiMode ? 2 : 1);
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
	LONG nPosLo = (uint16)pChn->nPosLo, nSmpCount = nSamples;
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
			nSmpCount = (ULONG)(((((int64)nPos - nLoopStart) << 16) + nPosLo - 1) / nInv) + 1;
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
			nSmpCount = (ULONG)(((((int64)pChn->nLength - nPos) << 16) - nPosLo - 1) / nInc) + 1;
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




void CSoundFile::CreateStereoMix(int count)
//-----------------------------------------
{
	LPLONG pOfsL, pOfsR;
	CHANNELINDEX nchused, nchmixed;

	if (!count) return;

	// Resetting sound buffer
	StereoFill(MixSoundBuffer, count, &gnDryROfsVol, &gnDryLOfsVol);
	if(m_MixerSettings.gnChannels > 2) InitMixBuffer(MixRearBuffer, count*2);

	bool ITPingPongMode = IsITPingPongMode();
	nchused = nchmixed = 0;
	for(CHANNELINDEX nChn=0; nChn<m_nMixChannels; nChn++)
	{
		const LPMIXINTERFACE *pMixFuncTable;
		ModChannel * const pChannel = &Chn[ChnMix[nChn]];
		UINT nFlags;
		LONG nSmpCount;
		int nsamples;
		int *pbuffer;

		if (!pChannel->pCurrentSample) continue;
		pOfsR = &gnDryROfsVol;
		pOfsL = &gnDryLOfsVol;
		nFlags = 0;
		if (pChannel->dwFlags[CHN_16BIT]) nFlags |= MIXNDX_16BIT;
		if (pChannel->dwFlags[CHN_STEREO]) nFlags |= MIXNDX_STEREO;
	#ifndef NO_FILTER
		if (pChannel->dwFlags[CHN_FILTER]) nFlags |= MIXNDX_FILTER;
	#endif
		//rewbs.resamplerConf
		nFlags |= ResamplingModeToMixFlags(pChannel->resamplingMode);
		//end rewbs.resamplerConf
		if ((nFlags < 0x20) && (pChannel->rightVol == pChannel->leftVol)
		 && ((!pChannel->nRampLength) || (pChannel->rightRamp == pChannel->leftRamp)))
		{
			pMixFuncTable = gpFastMixFunctionTable;
		} else
		{
			pMixFuncTable = gpMixFunctionTable;
		}
		nsamples = count;

		pbuffer = MixSoundBuffer;
#ifndef NO_REVERB
#ifdef ENABLE_MMX
		if(GetProcSupport() & PROCSUPPORT_MMX)
		{
			if(((m_MixerSettings.DSPMask & SNDDSP_REVERB) && !pChannel->dwFlags[CHN_NOREVERB]) || pChannel->dwFlags[CHN_REVERB])
			{
				pbuffer = m_Reverb.GetReverbSendBuffer(count);
				pOfsR = &m_Reverb.gnRvbROfsVol;
				pOfsL = &m_Reverb.gnRvbLOfsVol;
			}
		}
#endif
#endif
		if(pChannel->dwFlags[CHN_SURROUND] && m_MixerSettings.gnChannels > 2)
			pbuffer = MixRearBuffer;

		//Look for plugins associated with this implicit tracker channel.
		PLUGINDEX nMixPlugin = GetBestPlugin(ChnMix[nChn], PrioritiseInstrument, RespectMutes);

		if ((nMixPlugin > 0) && (nMixPlugin <= MAX_MIXPLUGINS))
		{
			SNDMIXPLUGINSTATE *pPlugin = m_MixPlugins[nMixPlugin - 1].pMixState;
			if ((pPlugin) && (pPlugin->pMixBuffer))
			{
				pbuffer = pPlugin->pMixBuffer;
				pOfsR = &pPlugin->nVolDecayR;
				pOfsL = &pPlugin->nVolDecayL;
				if (!(pPlugin->dwFlags & SNDMIXPLUGINSTATE::psfMixReady))
				{
					StereoFill(pbuffer, count, pOfsR, pOfsL);
					pPlugin->dwFlags |= SNDMIXPLUGINSTATE::psfMixReady;
				}
			}
		}
		nchused++;
		////////////////////////////////////////////////////
	SampleLooping:
		UINT nrampsamples = nsamples;
		if (pChannel->nRampLength > 0)
		{
			if ((LONG)nrampsamples > pChannel->nRampLength) nrampsamples = pChannel->nRampLength;
		}
		if ((nSmpCount = GetSampleCount(pChannel, nrampsamples, ITPingPongMode)) <= 0)
		{
			// Stopping the channel
			pChannel->pCurrentSample = NULL;
			pChannel->nLength = 0;
			pChannel->nPos = 0;
			pChannel->nPosLo = 0;
			pChannel->nRampLength = 0;
			EndChannelOfs(pChannel, pbuffer, nsamples);
			*pOfsR += pChannel->nROfs;
			*pOfsL += pChannel->nLOfs;
			pChannel->nROfs = pChannel->nLOfs = 0;
			pChannel->dwFlags.reset(CHN_PINGPONGFLAG);
			continue;
		}
		// Should we mix this channel ?
		bool addmix;
		if ((nchmixed >= m_MixerSettings.m_nMaxMixChannels)
		 || ((!pChannel->nRampLength) && (!(pChannel->rightVol|pChannel->leftVol))))
		{
			LONG delta = (pChannel->nInc * (LONG)nSmpCount) + (LONG)pChannel->nPosLo;
			pChannel->nPosLo = delta & 0xFFFF;
			pChannel->nPos += (delta >> 16);
			pChannel->nROfs = pChannel->nLOfs = 0;
			pbuffer += nSmpCount*2;
			addmix = false;
		} else
		// Do mixing
		{
			// Choose function for mixing
			LPMIXINTERFACE pMixFunc;
			pMixFunc = (pChannel->nRampLength) ? pMixFuncTable[nFlags|MIXNDX_RAMP] : pMixFuncTable[nFlags];
			int *pbufmax = pbuffer + (nSmpCount*2);
			pChannel->nROfs = - *(pbufmax-2);
			pChannel->nLOfs = - *(pbufmax-1);
			pMixFunc(pChannel, &m_Resampler, pbuffer, pbufmax);
			pChannel->nROfs += *(pbufmax-2);
			pChannel->nLOfs += *(pbufmax-1);
			pbuffer = pbufmax;
			addmix = true;
		}
		nsamples -= nSmpCount;
		if (pChannel->nRampLength)
		{
			pChannel->nRampLength -= nSmpCount;
			if (pChannel->nRampLength <= 0)
			{
				pChannel->nRampLength = 0;
				pChannel->leftVol = pChannel->newLeftVol;
				pChannel->rightVol = pChannel->newRightVol;
				pChannel->leftRamp = pChannel->rightRamp = 0;
				if(pChannel->dwFlags[CHN_NOTEFADE] && !pChannel->nFadeOutVol)
				{
					pChannel->nLength = 0;
					pChannel->pCurrentSample = nullptr;
				}
			}
		}
		if (nsamples > 0) goto SampleLooping;
		nchmixed += addmix?1:0;
	}
	m_nMixStat = std::max<CHANNELINDEX>(m_nMixStat, nchused);
}


void CSoundFile::ProcessPlugins(UINT nCount)
//------------------------------------------
{
	const float IntToFloat = m_PlayConfig.getIntToFloat();
	const float FloatToInt = m_PlayConfig.getFloatToInt();
	// Setup float inputs
	for(PLUGINDEX plug = 0; plug < MAX_MIXPLUGINS; plug++)
	{
		SNDMIXPLUGIN &plugin = m_MixPlugins[plug];
		if(plugin.pMixPlugin != nullptr && plugin.pMixState != nullptr
			&& plugin.pMixState->pMixBuffer != nullptr
			&& plugin.pMixState->pOutBufferL != nullptr
			&& plugin.pMixState->pOutBufferR != nullptr)
		{
			SNDMIXPLUGINSTATE *pState = plugin.pMixState;

			//We should only ever reach this point if the song is playing.
			if (!plugin.pMixPlugin->IsSongPlaying())
			{
				//Plugin doesn't know it is in a song that is playing;
				//we must have added it during playback. Initialise it!
				plugin.pMixPlugin->NotifySongPlaying(true);
				plugin.pMixPlugin->Resume();
			}


			// Setup float input
			if (pState->dwFlags & SNDMIXPLUGINSTATE::psfMixReady)
			{
				StereoMixToFloat(pState->pMixBuffer, pState->pOutBufferL, pState->pOutBufferR, nCount, IntToFloat);
			} else
			if (pState->nVolDecayR|pState->nVolDecayL)
			{
				StereoFill(pState->pMixBuffer, nCount, &pState->nVolDecayR, &pState->nVolDecayL);
				StereoMixToFloat(pState->pMixBuffer, pState->pOutBufferL, pState->pOutBufferR, nCount, IntToFloat);
			} else
			{
				memset(pState->pOutBufferL, 0, nCount * sizeof(float));
				memset(pState->pOutBufferR, 0, nCount * sizeof(float));
			}
			pState->dwFlags &= ~SNDMIXPLUGINSTATE::psfMixReady;
		}
	}
	// Convert mix buffer
	StereoMixToFloat(MixSoundBuffer, MixFloatBuffer, MixFloatBuffer + MIXBUFFERSIZE, nCount, IntToFloat);
	float *pMixL = MixFloatBuffer;
	float *pMixR = MixFloatBuffer + MIXBUFFERSIZE;

	// Process Plugins
	for(PLUGINDEX plug = 0; plug < MAX_MIXPLUGINS; plug++)
	{
		SNDMIXPLUGIN &plugin = m_MixPlugins[plug];
		if (plugin.pMixPlugin != nullptr && plugin.pMixState != nullptr
			&& plugin.pMixState->pMixBuffer != nullptr
			&& plugin.pMixState->pOutBufferL != nullptr
			&& plugin.pMixState->pOutBufferR != nullptr)
		{
			bool isMasterMix = false;
			if (pMixL == plugin.pMixState->pOutBufferL)
			{
				isMasterMix = true;
				pMixL = MixFloatBuffer;
				pMixR = MixFloatBuffer + MIXBUFFERSIZE;
			}
			IMixPlugin *pObject = plugin.pMixPlugin;
			SNDMIXPLUGINSTATE *pState = plugin.pMixState;
			float *pOutL = pMixL;
			float *pOutR = pMixR;

			if (!plugin.IsOutputToMaster())
			{
				PLUGINDEX nOutput = plugin.GetOutputPlugin();
				if(nOutput > plug && nOutput != PLUGINDEX_INVALID
					&& m_MixPlugins[nOutput].pMixState != nullptr)
				{
					SNDMIXPLUGINSTATE *pOutState = m_MixPlugins[nOutput].pMixState;

					if(pOutState->pOutBufferL != nullptr && pOutState->pOutBufferR != nullptr)
					{
						pOutL = pOutState->pOutBufferL;
						pOutR = pOutState->pOutBufferR;
					}
				}
			}

			/*
			if (plugin.multiRouting) {
				int nOutput=0;
				for (int nOutput=0; nOutput < plugin.nOutputs / 2; nOutput++) {
					destinationPlug = plugin.multiRoutingDestinations[nOutput];
					pOutState = m_MixPlugins[destinationPlug].pMixState;
					pOutputs[2 * nOutput] = pOutState->pOutBufferL;
					pOutputs[2 * (nOutput + 1)] = pOutState->pOutBufferR;
				}

			}*/

			if (plugin.IsMasterEffect())
			{
				if (!isMasterMix)
				{
					float *pInL = pState->pOutBufferL;
					float *pInR = pState->pOutBufferR;
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

			if (plugin.IsBypassed())
			{
				const float * const pInL = pState->pOutBufferL;
				const float * const pInR = pState->pOutBufferR;
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
	FloatToStereoMix(pMixL, pMixR, MixSoundBuffer, nCount, FloatToInt);
}

