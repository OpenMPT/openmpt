/*
 * This program is  free software; you can redistribute it  and modify it
 * under the terms of the GNU  General Public License as published by the
 * Free Software Foundation; either version 2  of the license or (at your
 * option) any later version.
 *
 * Authors: Olivier Lapicque <olivierl@jps.net>
 *          Rani Assaf <rani@magic.metawire.com> (C translations of asm)
 *          Kenton Varda <temporal@gauge3d.org> (GCC port)
 *          Markus Fick <webmaster@mark-f.de> (changed SNDMIX_PROCESSFILTER from 2p1z to 2p, WIN32 ifdefs)
*/

#include "stdafx.h"
#include "sndfile.h"

//#pragma bss_seg(".modplug")

// Front Mix Buffer (Also room for interleaved rear mix)
int MixSoundBuffer[MIXBUFFERSIZE*4];

// Reverb Mix Buffer
#ifndef NO_REVERB
int MixReverbBuffer[MIXBUFFERSIZE*2];
extern UINT gnReverbSend;
#endif

//#pragma bss_seg()

/////////////////////////////////////////////////////
// Mixing Macros

#define SNDMIX_BEGINSAMPLELOOP8\
	register MODCHANNEL *pChn = pChannel;\
	nPos = pChn->nPosLo;\
	signed char *p = (signed char *)(pChn->pCurrentSample+pChn->nPos);\
	int *pvol = pbuffer;\
	do {

#define SNDMIX_BEGINSAMPLELOOP16\
	register MODCHANNEL *pChn = pChannel;\
	nPos = pChn->nPosLo;\
	signed short *p = (signed short *)(pChn->pCurrentSample+(pChn->nPos<<1));\
	int *pvol = pbuffer;\
	do {

#define SNDMIX_ENDSAMPLELOOP\
		nPos += pChn->nInc;\
	} while (pvol < pbufmax);\
	pChn->nPos += nPos >> 16;\
	pChn->nPosLo = nPos & 0xFFFF;

#define SNDMIX_ENDSAMPLELOOP8	SNDMIX_ENDSAMPLELOOP
#define SNDMIX_ENDSAMPLELOOP16	SNDMIX_ENDSAMPLELOOP


#define SNDMIX_GETVOL8NOIDO\
	vol = p[nPos >> 16] << 8;

#define SNDMIX_GETVOL16NOIDO\
	vol = p[nPos >> 16];

#define SNDMIX_GETVOL8LINEAR\
	int poshi = nPos >> 16;\
	int poslo = (nPos >> 8) & 0xFF;\
	int srcvol = p[poshi];\
	int destvol = p[poshi+1];\
	vol = (srcvol<<8) + ((int)(poslo * (destvol - srcvol)));

#define SNDMIX_GETVOL16LINEAR\
	int poshi = nPos >> 16;\
	int poslo = (nPos >> 8) & 0xFF;\
	int srcvol = p[poshi];\
	int destvol = p[poshi+1];\
	vol = srcvol + ((int)(poslo * (destvol - srcvol)) >> 8);


#define SNDMIX_STOREMONOVOL\
	*pvol++ += vol * pChn->nRightVol;

#define SNDMIX_STORESTEREOVOL\
	pvol[0] += vol * pChn->nRightVol;\
	pvol[1] += vol * pChn->nLeftVol;\
	pvol += 2;

#define SNDMIX_STOREFASTSTEREOVOL\
	int v = vol * pChn->nRightVol;\
	pvol[0] += v;\
	pvol[1] += v;\
	pvol += 2;

#define SNDMIX_VOLUMERAMPMONO\
	nRampRightVol += pChn->nRightRamp;\
	*pvol++ += vol * (nRampRightVol >> VOLUMERAMPPRECISION);

#define SNDMIX_VOLUMERAMPSTEREO\
	nRampRightVol += pChn->nRightRamp;\
	nRampLeftVol += pChn->nLeftRamp;\
	pvol[0] += vol * (nRampRightVol >> VOLUMERAMPPRECISION);\
	pvol[1] += vol * (nRampLeftVol >> VOLUMERAMPPRECISION);\
	pvol += 2;


///////////////////////////////////////////////////
// Resonant Filters

#define MIX_BEGIN_FILTER\
	int fx1 = pChannel->nFilter_X1;\
	int fx2 = pChannel->nFilter_X2;\

#define MIX_END_FILTER\
	pChannel->nFilter_X1 = fx1;\
	pChannel->nFilter_X2 = fx2;

#define SNDMIX_PROCESSFILTER\
	vol = (vol * pChn->nFilter_B0 + fx1 * pChn->nFilter_B1 + fx2 * pChn->nFilter_B2) / 8192;\
	fx2 = fx1;\
	fx1 = vol;\

//////////////////////////////////////////////////////////
// Interfaces

typedef LONG (MPPASMCALL * LPMIXINTERFACE)(MODCHANNEL *, int *, int *);

#define BEGIN_MIX_INTERFACE(func)\
	LONG MPPASMCALL func(MODCHANNEL *pChannel, int *pbuffer, int *pbufmax)\
	{\
		LONG vol, nPos;

#define END_MIX_INTERFACE()\
		SNDMIX_ENDSAMPLELOOP\
		return vol;\
	}

// Resonant Filter
#define BEGIN_MIX_FLT_INTERFACE(func)\
	BEGIN_MIX_INTERFACE(func)\
	MIX_BEGIN_FILTER
	

#define END_MIX_FLT_INTERFACE()\
	SNDMIX_ENDSAMPLELOOP\
	MIX_END_FILTER\
	return vol;\
	}


// Stereo Volume Ramp
#define BEGIN_RAMP_STEREO(func)\
	BEGIN_MIX_INTERFACE(func)\
		LONG nRampRightVol = pChannel->nRampRightVol;\
		LONG nRampLeftVol = pChannel->nRampLeftVol;


#define END_RAMP_STEREO()\
		SNDMIX_ENDSAMPLELOOP\
		pChannel->nRampRightVol = nRampRightVol;\
		pChannel->nRightVol = nRampRightVol >> VOLUMERAMPPRECISION;\
		pChannel->nRampLeftVol = nRampLeftVol;\
		pChannel->nLeftVol = nRampLeftVol >> VOLUMERAMPPRECISION;\
		return vol;\
	}


// Mono Volume Ramp
#define BEGIN_RAMP_MONO(func)\
	BEGIN_MIX_INTERFACE(func)\
		LONG nRampRightVol = pChannel->nRampRightVol;\

#define END_RAMP_MONO()\
		SNDMIX_ENDSAMPLELOOP\
		pChannel->nRampRightVol = nRampRightVol;\
		pChannel->nRightVol = nRampRightVol >> VOLUMERAMPPRECISION;\
		return vol;\
	}


// Stereo Filtered Volume Ramp
#define BEGIN_RAMPMIX_FLT_STEREO(func)\
	BEGIN_MIX_INTERFACE(func)\
		LONG nRampRightVol = pChannel->nRampRightVol;\
		LONG nRampLeftVol = pChannel->nRampLeftVol;\
		MIX_BEGIN_FILTER

#define END_RAMPMIX_FLT_STEREO()\
		SNDMIX_ENDSAMPLELOOP\
		MIX_END_FILTER\
		pChannel->nRampRightVol = nRampRightVol;\
		pChannel->nRightVol = nRampRightVol >> VOLUMERAMPPRECISION;\
		pChannel->nRampLeftVol = nRampLeftVol;\
		pChannel->nLeftVol = nRampLeftVol >> VOLUMERAMPPRECISION;\
		return vol;\
	}


// Mono Filtered Volume Ramp
#define BEGIN_RAMPMIX_FLT_MONO(func)\
	BEGIN_MIX_INTERFACE(func)\
		LONG nRampRightVol = pChannel->nRampRightVol;\
		MIX_BEGIN_FILTER\

#define END_RAMPMIX_FLT_MONO()\
		SNDMIX_ENDSAMPLELOOP\
		MIX_END_FILTER\
		pChannel->nRampRightVol = nRampRightVol;\
		pChannel->nRightVol = nRampRightVol >> VOLUMERAMPPRECISION;\
		return vol;\
	}


/////////////////////////////////////////////////////
// Stereo Mix functions

BEGIN_MIX_INTERFACE(Stereo8BitMix)
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETVOL8NOIDO
	SNDMIX_STORESTEREOVOL
END_MIX_INTERFACE()

BEGIN_MIX_INTERFACE(Stereo16BitMix)
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETVOL16NOIDO
	SNDMIX_STORESTEREOVOL
END_MIX_INTERFACE()

BEGIN_MIX_INTERFACE(Stereo8BitLinearMix)
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETVOL8LINEAR
	SNDMIX_STORESTEREOVOL
END_MIX_INTERFACE()

BEGIN_MIX_INTERFACE(Stereo16BitLinearMix)
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETVOL16LINEAR
	SNDMIX_STORESTEREOVOL
END_MIX_INTERFACE()


// Stereo Ramps
BEGIN_RAMP_STEREO(Stereo8BitRampMix)
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETVOL8NOIDO
	SNDMIX_VOLUMERAMPSTEREO
END_RAMP_STEREO()

BEGIN_RAMP_STEREO(Stereo16BitRampMix)
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETVOL16NOIDO
	SNDMIX_VOLUMERAMPSTEREO
END_RAMP_STEREO()

BEGIN_RAMP_STEREO(Stereo8BitLinearRampMix)
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETVOL8LINEAR
	SNDMIX_VOLUMERAMPSTEREO
END_RAMP_STEREO()

BEGIN_RAMP_STEREO(Stereo16BitLinearRampMix)
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETVOL16LINEAR
	SNDMIX_VOLUMERAMPSTEREO
END_RAMP_STEREO()


const LPMIXINTERFACE gpStereoMix[] =
{
	Stereo8BitMix,
	Stereo16BitMix,
	Stereo8BitLinearMix,
	Stereo16BitLinearMix,
	Stereo8BitRampMix,
	Stereo16BitRampMix,
	Stereo8BitLinearRampMix,
	Stereo16BitLinearRampMix,
};


//////////////////////////////////////////////////////
// Fast Stereo Mix for mono channels (1 less imul)

BEGIN_MIX_INTERFACE(FastStereo8BitMix)
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETVOL8NOIDO
	SNDMIX_STOREFASTSTEREOVOL
END_MIX_INTERFACE()

BEGIN_MIX_INTERFACE(FastStereo16BitMix)
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETVOL16NOIDO
	SNDMIX_STOREFASTSTEREOVOL
END_MIX_INTERFACE()

BEGIN_MIX_INTERFACE(FastStereo8BitLinearMix)
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETVOL8LINEAR
	SNDMIX_STOREFASTSTEREOVOL
END_MIX_INTERFACE()

BEGIN_MIX_INTERFACE(FastStereo16BitLinearMix)
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETVOL16LINEAR
	SNDMIX_STOREFASTSTEREOVOL
END_MIX_INTERFACE()


const LPMIXINTERFACE gpFastStereoMix[] =
{
	FastStereo8BitMix,
	FastStereo16BitMix,
	FastStereo8BitLinearMix,
	FastStereo16BitLinearMix
};

//////////////////////////////////////////////////////
// Resonant Filter Mix

#ifndef NO_FILTER

// Stereo Filter Mix
BEGIN_MIX_FLT_INTERFACE(FilterStereo8BitMix)
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETVOL8NOIDO
	SNDMIX_PROCESSFILTER
	SNDMIX_STORESTEREOVOL
END_MIX_FLT_INTERFACE()

BEGIN_MIX_FLT_INTERFACE(FilterStereo16BitMix)
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETVOL16NOIDO
	SNDMIX_PROCESSFILTER
	SNDMIX_STORESTEREOVOL
END_MIX_FLT_INTERFACE()

BEGIN_RAMPMIX_FLT_STEREO(FilterStereo8BitRampMix)
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETVOL8NOIDO
	SNDMIX_PROCESSFILTER
	SNDMIX_VOLUMERAMPSTEREO
END_RAMPMIX_FLT_STEREO()

BEGIN_RAMPMIX_FLT_STEREO(FilterStereo16BitRampMix)
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETVOL16NOIDO
	SNDMIX_PROCESSFILTER
	SNDMIX_VOLUMERAMPSTEREO
END_RAMPMIX_FLT_STEREO()


const LPMIXINTERFACE gpFilterStereoMix[] =
{
	FilterStereo8BitMix,
	FilterStereo16BitMix,
	FilterStereo8BitRampMix,
	FilterStereo16BitRampMix,
};

// Mono Filter Mix
BEGIN_MIX_FLT_INTERFACE(FilterMono8BitMix)
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETVOL8NOIDO
	SNDMIX_PROCESSFILTER
	SNDMIX_STOREMONOVOL
END_MIX_FLT_INTERFACE()

BEGIN_MIX_FLT_INTERFACE(FilterMono16BitMix)
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETVOL16NOIDO
	SNDMIX_PROCESSFILTER
	SNDMIX_STOREMONOVOL
END_MIX_FLT_INTERFACE()

BEGIN_RAMPMIX_FLT_MONO(FilterMono8BitRampMix)
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETVOL8NOIDO
	SNDMIX_PROCESSFILTER
	SNDMIX_VOLUMERAMPMONO
END_RAMPMIX_FLT_MONO()

BEGIN_RAMPMIX_FLT_MONO(FilterMono16BitRampMix)
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETVOL16NOIDO
	SNDMIX_PROCESSFILTER
	SNDMIX_VOLUMERAMPMONO
END_RAMPMIX_FLT_MONO()


const LPMIXINTERFACE gpFilterMonoMix[] =
{
	FilterMono8BitMix,
	FilterMono16BitMix,
	FilterMono8BitRampMix,
	FilterMono16BitRampMix,
};

#endif // NO_FILTER

/////////////////////////////////////////////////////
// Mono Mix functions

BEGIN_MIX_INTERFACE(Mono8BitMix)
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETVOL8NOIDO
	SNDMIX_STOREMONOVOL
END_MIX_INTERFACE()

BEGIN_MIX_INTERFACE(Mono16BitMix)
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETVOL16NOIDO
	SNDMIX_STOREMONOVOL
END_MIX_INTERFACE()

BEGIN_MIX_INTERFACE(Mono8BitLinearMix)
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETVOL8LINEAR
	SNDMIX_STOREMONOVOL
END_MIX_INTERFACE()

BEGIN_MIX_INTERFACE(Mono16BitLinearMix)
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETVOL16LINEAR
	SNDMIX_STOREMONOVOL
END_MIX_INTERFACE()


// Mono Ramps
BEGIN_RAMP_MONO(Mono8BitRampMix)
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETVOL8NOIDO
	SNDMIX_VOLUMERAMPMONO
END_RAMP_MONO()

BEGIN_RAMP_MONO(Mono16BitRampMix)
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETVOL16NOIDO
	SNDMIX_VOLUMERAMPMONO
END_RAMP_MONO()

BEGIN_RAMP_MONO(Mono8BitLinearRampMix)
	SNDMIX_BEGINSAMPLELOOP8
	SNDMIX_GETVOL8LINEAR
	SNDMIX_VOLUMERAMPMONO
END_RAMP_MONO()

BEGIN_RAMP_MONO(Mono16BitLinearRampMix)
	SNDMIX_BEGINSAMPLELOOP16
	SNDMIX_GETVOL16LINEAR
	SNDMIX_VOLUMERAMPMONO
END_RAMP_MONO()


const LPMIXINTERFACE gpMonoMix[] =
{
	Mono8BitMix,
	Mono16BitMix,
	Mono8BitLinearMix,
	Mono16BitLinearMix,
	Mono8BitRampMix,
	Mono16BitRampMix,
	Mono8BitLinearRampMix,
	Mono16BitLinearRampMix,
};



/////////////////////////////////////////////////////////////////////////

static LONG MPPFASTCALL GetSampleCount(MODCHANNEL *pChn, LONG nSamples, DWORD nType)
//----------------------------------------------------------------------------------
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
			if ((pChn->nPos <= pChn->nLoopStart) || (pChn->nPos >= pChn->nLength))
				pChn->nPos = pChn->nLength-1;
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

/* old version
static LONG MPPFASTCALL GetSampleCount(MODCHANNEL *pChn, LONG nSamples, DWORD nType)
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
			nInc = -nInc;
			pChn->nInc = nInc;
			pChn->dwFlags &= ~(CHN_PINGPONGFLAG); // go forward
			if (!(pChn->dwFlags & CHN_LOOP)) return 0;
			if (nType & MOD_TYPE_XM)
			{
				pChn->nPos += pChn->nInc >> 16;
				if ((pChn->nPos < pChn->nLoopStart) || (pChn->nPos >= pChn->nLength)) pChn->nPos = pChn->nLoopStart;
			} else pChn->nPos = nLoopStart;
			pChn->nPosLo = 0;
			if (pChn->nPos >= pChn->nLength) return 0; // shouldn't happen
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
			if (nType & MOD_TYPE_XM)
			{
				pChn->nPos += pChn->nInc >> 16;
				if ((pChn->nPos < pChn->nLoopStart) || (pChn->nPos >= pChn->nLength)) pChn->nPos = pChn->nLength-1;
			} else pChn->nPos = pChn->nLength - 1;
			pChn->nPosLo = 0xFFFF;
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
	if (nPos >= (LONG)pChn->nLength) return 0;
	LONG nPosLo = pChn->nPosLo, nSmpCount = nSamples;
	if (nInc < 0)
	{
		LONG nInv = -nInc;
		LONG nDelta = (nPosLo >> 2) + (nInv >> 2) * (nSamples - 1);
		LONG nPosDest = nPos - (nDelta >> 14);
		if (nPosDest < nLoopStart)
		{
			nSmpCount = ((((nPos - nLoopStart) << 16) + nPosLo) / nInv) + 1;
		}
	} else
	{
		LONG nDelta = (nPosLo >> 2) + (nInc >> 2) * (nSamples - 1);
		LONG nPosDest = nPos + (nDelta >> 14);
		if (nPosDest >= (LONG)pChn->nLength)
		{
			nSmpCount = (((((LONG)pChn->nLength - nPos) << 16) - nPosLo - 1) / nInc) + 1;
		}
	}
	if (nSmpCount <= 1) return 1;
	if (nSmpCount > nSamples) return nSamples;
	return nSmpCount;
}
*/

#define CHNRAMPNDX	0x04

UINT CSoundFile::CreateStereoMix(int count)
//-----------------------------------------
{
	const LPMIXINTERFACE *pStereoMixFunc;
	DWORD nchused, nchmixed;
	
	if (!count) return 0;
	pStereoMixFunc = gpStereoMix;
	nchused = nchmixed = 0;
	for (UINT nChn=0; nChn<m_nMixChannels; nChn++)
	{
		MODCHANNEL *pChannel = ChnMix[nChn];
		LONG nSmpCount;
		int nsamples;
		int *pbuffer;

		if (!pChannel->pCurrentSample) continue;
		UINT nFlags = (pChannel->dwFlags & CHN_16BIT) ? 1 : 0;
		if (!(pChannel->dwFlags & CHN_NOIDO)) nFlags |= 0x02;
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
		if ((nSmpCount = GetSampleCount(pChannel, nrampsamples, m_nType)) <= 0)
		{
			// Stopping the channel
			LONG rofsvol = pChannel->nROfs & OFSVOLDECMASK;
			LONG lofsvol = pChannel->nLOfs & OFSVOLDECMASK;
			int rvcd = (rofsvol < 0) ? OFSVOLDECSPEED : -OFSVOLDECSPEED;
			int lvcd = (lofsvol < 0) ? OFSVOLDECSPEED : -OFSVOLDECSPEED;
			while (nsamples-- > 0)
			{
				if (rofsvol)
				{
					rofsvol += rvcd;
					if (lofsvol) lofsvol += lvcd;
				} else
				{
					if (!lofsvol) break;
					lofsvol += lvcd;
				}
				pbuffer[0] += rofsvol;
				pbuffer[1] += lofsvol;
				pbuffer += 2;
			}
			pChannel->pCurrentSample = NULL;
			pChannel->nLength = 0;
			pChannel->nPos = 0;
			pChannel->nPosLo = 0;
			pChannel->nRampLength = 0;
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
			pbuffer += (nSmpCount << 1);
			naddmix = 0;
		} else
		// Do mixing
		{
			// Choose function for mixing
			LPMIXINTERFACE pMixFunc;
#ifndef NO_FILTER
			if (pChannel->dwFlags & CHN_FILTER)
			{
				UINT nFltFlag = nFlags & 1;
				if (pChannel->nRampLength) nFltFlag |= 2;
				pMixFunc = gpFilterStereoMix[nFltFlag];
			} else
#endif // NO_FILTER
			{
				if (pChannel->nRampLength)
				{
					pMixFunc = gpStereoMix[nFlags|CHNRAMPNDX];
				} else
				{
					pMixFunc = pStereoMixFunc[nFlags];
					// Fast for non-MMX
					if ((nFlags < 4) && (pChannel->nLeftVol == pChannel->nRightVol) && (pStereoMixFunc == gpStereoMix))
					{
						pMixFunc = gpFastStereoMix[nFlags];
					}
				}
			}

			int *pbufmax = pbuffer + (nSmpCount << 1);
			LONG vol = pMixFunc(pChannel, pbuffer, pbufmax);
			pChannel->nROfs = vol * pChannel->nRightVol;
			pChannel->nLOfs = vol * pChannel->nLeftVol;
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


UINT CSoundFile::CreateMonoMix(int count)
//---------------------------------------
{
	DWORD nchused, nchmixed;

	if (!count) return 0;
	nchused = nchmixed = 0;
	for (UINT nChn=0; nChn<m_nMixChannels; nChn++)
	{
		MODCHANNEL *pChannel = ChnMix[nChn];
		LONG nSmpCount;
		int nsamples;
		int *pbuffer;

		if (!pChannel->pCurrentSample) continue;
		UINT nFlags = (pChannel->dwFlags & CHN_16BIT) ? 1 : 0;
		if (!(pChannel->dwFlags & CHN_NOIDO)) nFlags |= 0x02;
		nsamples = count;
#ifndef NO_REVERB
		pbuffer = (gdwSoundSetup & SNDMIX_REVERB) ? MixReverbBuffer : MixSoundBuffer;
		if (pChannel->dwFlags & CHN_NOREVERB) pbuffer = MixSoundBuffer;
		if (pChannel->dwFlags & CHN_REVERB) pbuffer = MixReverbBuffer;
		if (pbuffer == MixReverbBuffer)
		{
			if (!gnReverbSend) memset(MixReverbBuffer, 0, count * 4);
			gnReverbSend += count;
		}
#else
		pbuffer = MixSoundBuffer;
#endif
		nchused++;
	SampleLooping:
		UINT nrampsamples = nsamples;
		if (pChannel->nRampLength > 0)
		{
			if ((LONG)nrampsamples > pChannel->nRampLength) nrampsamples = pChannel->nRampLength;
		}
		if ((nSmpCount = GetSampleCount(pChannel, nrampsamples, m_nType)) <= 0)
		{
			LONG ofsvol = pChannel->nROfs & OFSVOLDECMASK;
			int vcd = (ofsvol < 0) ? OFSVOLDECSPEED : -OFSVOLDECSPEED;
			while ((nsamples-- > 0) && (ofsvol))
			{
				ofsvol += vcd;
				*pbuffer++ += ofsvol;
			}
			gnROfsVol += ofsvol;
			gnLOfsVol += ofsvol;
			pChannel->pCurrentSample = NULL;
			pChannel->nLength = 0;
			pChannel->nPos = 0;
			pChannel->nPosLo = 0;
			pChannel->nRampLength = 0;
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
			pbuffer += nSmpCount;
			naddmix = 0;
		} else
		// Mix this channel
		{
			LPMIXINTERFACE pMixFunc;

#ifndef NO_FILTER
			if (pChannel->dwFlags & CHN_FILTER)
			{
				UINT nFltFlag = nFlags & 1;
				pMixFunc = (pChannel->nRampLength) ? gpFilterMonoMix[nFltFlag|2] : gpFilterMonoMix[nFltFlag];
			} else
#endif // NO_FILTER
			{
				pMixFunc = (pChannel->nRampLength) ? gpMonoMix[nFlags|CHNRAMPNDX] : gpMonoMix[nFlags];
			}

			int *pbufmax = pbuffer + nSmpCount;
			LONG vol = pMixFunc(pChannel, pbuffer, pbufmax);
			pChannel->nLOfs = pChannel->nROfs = vol * pChannel->nRightVol;
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


//#pragma warning (disable:4100)

// Clip and convert to 8/16 bit


//---GCCFIX: Asm replaced with C functions
// The C versions were written by Rani Assaf <rani@magic.metawire.com>, I believe
//TODO: Convert asm to GCC-style!
#ifdef WIN32
DWORD MPPASMCALL X86_Convert32To8(LPVOID lp8, int *pBuffer, DWORD lSampleCount, LPLONG lpMin, LPLONG lpMax)
#else
__declspec(naked) DWORD MPPASMCALL X86_Convert32To8(LPVOID lp8, int *pBuffer, DWORD lSampleCount, LPLONG lpMin, LPLONG lpMax)
#endif
//----------------------------------------------------------------------------------------------------------------------------
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

/*
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
*/
}


// Perform clipping
#ifdef WIN32
DWORD MPPASMCALL X86_Convert32To16(LPVOID lp16, int *pBuffer, DWORD lSampleCount, LPLONG lpMin, LPLONG lpMax)
#else
__declspec(naked) DWORD MPPASMCALL X86_Convert32To16(LPVOID lp16, int *pBuffer, DWORD lSampleCount, LPLONG lpMin, LPLONG lpMax)
#endif
//-----------------------------------------------------------------------------------------------------------------------------
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

/*
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
*/
}


#ifdef WIN32
void MPPASMCALL X86_StereoFill(int *pBuffer, UINT nSamples, LPLONG lpROfs, LPLONG lpLOfs)
#else
__declspec(naked) void MPPASMCALL X86_StereoFill(int *pBuffer, UINT nSamples, LPLONG lpROfs, LPLONG lpLOfs)
#endif
//---------------------------------------------------------------------------------------------------------
{
	int ofsr = (*lpROfs) & OFSVOLDECMASK, ofsl = (*lpLOfs) & OFSVOLDECMASK;
	for (UINT i=0; i<nSamples; i++, pBuffer += 2)
	{
		if (ofsr > OFSVOLDECSPEED)
			ofsr -= OFSVOLDECSPEED;
		else if (ofsr < -OFSVOLDECSPEED)
			ofsr += OFSVOLDECSPEED;
		if (ofsl > OFSVOLDECSPEED)
			ofsl -= OFSVOLDECSPEED;
		else if (ofsl < -OFSVOLDECSPEED)
			ofsl += OFSVOLDECSPEED;
		pBuffer[0] = ofsr;
		pBuffer[1] = ofsl;
	}
	*lpROfs = ofsr;
	*lpLOfs = ofsl;

/*
	_asm {
		push ebx
		push ebp
		push esi
		push edi
		mov esi, 20[esp]
		mov ebp, 24[esp]
		mov eax, 28[esp]
		mov ecx, dword ptr [eax]	// ecx = Right Offset
		mov ebx, -OFSVOLDECSPEED	// ebx = Right Offset Vol Delta
		mov eax, 32[esp]
		and ecx, OFSVOLDECMASK
		mov edx, dword ptr [eax]
		mov edi, -OFSVOLDECSPEED	// edi = Left Offset Vol Delta
		and edx, OFSVOLDECMASK		// edx = Left offset
		test ecx, ecx
		jl right_ready
		neg ebx
		test ecx, ecx
		jg right_ready
		xor ebx, ebx
	right_ready:
		test edx, edx
		jl left_ready
		neg edi
		test edx, edx
		jg left_ready
		xor edi, edi
	left_ready:
	fill_loop:
		sub ecx, ebx
		jc right_zero
	right_recover:
		sub edx, edi
		jc left_zero
	left_recover:
		add esi, 8
		dec ebp
		mov dword ptr [esi-8], ecx
		mov dword ptr [esi-4], edx
		jnz fill_loop
		mov eax, dword ptr 28[esp]
		mov dword ptr [eax], ecx
		mov eax, dword ptr 32[esp]
		pop edi
		mov dword ptr [eax], edx
		pop esi
		pop ebp
		pop ebx
		ret
	right_zero:
		xor ecx, ecx
		xor ebx, ebx
		jmp right_recover
	left_zero:
		xor edx, edx
		xor edi, edi
		jmp left_recover
	}
*/
}

#ifdef WIN32
void MPPASMCALL X86_MonoFill(int *pBuffer, UINT nSamples, LPLONG lpROfs)
#else
__declspec(naked) void MPPASMCALL X86_MonoFill(int *pBuffer, UINT nSamples, LPLONG lpROfs)
#endif
//----------------------------------------------------------------------------------------
{
	int ofs = (*lpROfs) & OFSVOLDECMASK;
	for (UINT i=0; i<nSamples; i++, pBuffer++)
	{
		if (ofs > OFSVOLDECSPEED)
			ofs -= OFSVOLDECSPEED;
		else if (ofs < -OFSVOLDECSPEED)
			ofs += OFSVOLDECSPEED;
		*pBuffer = ofs;
	}
	*lpROfs = ofs;

/*
	_asm {
		push ebp
		push esi
		mov esi, 12[esp]
		mov ebp, 16[esp]
		mov eax, 20[esp]
		mov edx, -OFSVOLDECSPEED	// edx = Offset Vol Delta
		mov ecx, dword ptr [eax]	// ecx = Offset
		and ecx, OFSVOLDECMASK
		test ecx, ecx
		jl fill_loop
		neg edx
		test ecx, ecx
		jg fill_loop
		xor edx, edx
	fill_loop:
		sub ecx, edx
		jc mono_zero
	mono_recover:
		add esi, 4
		dec ebp
		mov dword ptr [esi-4], edx
		jnz fill_loop
		mov eax, dword ptr 20[esp]
		pop esi
		mov dword ptr [eax], ecx
		pop ebp
		ret
	mono_zero:
		xor ecx, ecx
		xor edx, edx
		jmp mono_recover
	}
*/
}


//////////////////////////////////////////////////////////////////////////////////
// Automatic Gain Control

#ifndef NO_AGC

// Limiter
#define MIXING_LIMITMAX		(0x08080000)
#define MIXING_LIMITMIN		(-MIXING_LIMITMAX)

#ifdef WIN32
UINT MPPASMCALL X86_AGC(int *pBuffer, UINT nSamples, UINT nAGC)
#else
__declspec(naked) UINT MPPASMCALL X86_AGC(int *pBuffer, UINT nSamples, UINT nAGC)
#endif
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

//---END GCCFIX

//#pragma warning (default:4100)

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



