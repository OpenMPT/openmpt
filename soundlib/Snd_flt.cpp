/*
 * OpenMPT
 *
 * Snd_flt.cpp
 *
 * Authors: Olivier Lapicque <olivierl@jps.net>
 *          OpenMPT devs
*/

#include "stdafx.h"
#include "sndfile.h"

// AWE32: cutoff = reg[0-255] * 31.25 + 100 -> [100Hz-8060Hz]
// EMU10K1 docs: cutoff = reg[0-127]*62+100
#define FILTER_PRECISION	8192

#ifndef NO_FILTER

#define _USE_MATH_DEFINES
#include <math.h>

extern float ITResonanceTable[128];


DWORD CSoundFile::CutOffToFrequency(UINT nCutOff, int flt_modifier) const
//-----------------------------------------------------------------------
{
	float Fc;
	ASSERT(nCutOff < 128);
	if (m_dwSongFlags & SONG_EXFILTERRANGE)
		Fc = 110.0f * pow(2.0f, 0.25f + ((float)(nCutOff * (flt_modifier + 256))) / (20.0f * 512.0f));
	else
		Fc = 110.0f * pow(2.0f, 0.25f + ((float)(nCutOff * (flt_modifier + 256))) / (24.0f * 512.0f));
	LONG freq = (LONG)Fc;
	Limit(freq, 120, 20000);
	if (freq * 2 > (LONG)gdwMixingFreq) freq = gdwMixingFreq >> 1;
	return (DWORD)freq;
}


// Simple 2-poles resonant filter
void CSoundFile::SetupChannelFilter(MODCHANNEL *pChn, bool bReset, int flt_modifier) const
//----------------------------------------------------------------------------------------
{
	int cutoff = (int)pChn->nCutOff + (int)pChn->nCutSwing;
	int resonance = (int)(pChn->nResonance & 0x7F) + (int)pChn->nResSwing;

	Limit(cutoff, 0, 127);
	Limit(resonance, 0, 127);

	if(!GetModFlag(MSF_OLDVOLSWING))
	{
		pChn->nCutOff = (BYTE)cutoff;
		pChn->nCutSwing = 0;
		pChn->nResonance = (BYTE)resonance;
		pChn->nResSwing = 0;
	}

	float d, e;

	if(UseITFilterMode())
	{

		// flt_modifier is in [-256, 256], so cutoff is in [0, 127 * 2] after this calculation
		cutoff = cutoff * (flt_modifier + 256) / 256;

		// Filtering is only ever done if either cutoff is not full or if resonance is set.
		if (cutoff < 254 || resonance != 0)
		{
			pChn->dwFlags |= CHN_FILTER;
		} else
		{
			return;
		}

		static const float freqMultiplier = 2.0f * (float)M_PI * 110.0f * pow(2.0f, 0.25f);
		static const float freqParameterMultiplier = 128.0f / (24.0f * 256.0f);

		// 2 ^ (i / 24 * 256)
		const float r = (float)gdwMixingFreq / (freqMultiplier * pow(2.0f, (float)cutoff * freqParameterMultiplier));

		d = ITResonanceTable[resonance] * r + ITResonanceTable[resonance] - 1.0f;
		e = r * r;

	} else
	{

		pChn->dwFlags |= CHN_FILTER;

		float fc = (float)CutOffToFrequency(cutoff, flt_modifier);
		const float dmpfac = pow(10.0f, -((24.0f / 128.0f) * (float)resonance) / 20.0f);

		fc *= (float)(2.0 * 3.14159265358 / (float)gdwMixingFreq);

		d = (1.0f - 2.0f * dmpfac) * fc;
		LimitMax(d, 2.0f);
		d = (2.0f * dmpfac - d) / fc;
		e = pow(1.0f / fc, 2.0f);

	}

	float fg = 1 / (1 + d + e);
	float fb0 = (d + e + e) / (1 + d + e);
	float fb1 = -e / (1 + d + e);

	switch(pChn->nFilterMode)
	{
	case FLTMODE_HIGHPASS:
		pChn->nFilter_A0 = (int)((1.0f - fg) * FILTER_PRECISION);
		pChn->nFilter_B0 = (int)(fb0 * FILTER_PRECISION);
		pChn->nFilter_B1 = (int)(fb1 * FILTER_PRECISION);
		pChn->nFilter_HP = -1;
		break;

	default:
		pChn->nFilter_A0 = (int)(fg * FILTER_PRECISION);
		pChn->nFilter_B0 = (int)(fb0 * FILTER_PRECISION);
		pChn->nFilter_B1 = (int)(fb1 * FILTER_PRECISION);
		pChn->nFilter_HP = 0;
		break;
	}
	
	if (bReset)
	{
		pChn->nFilter_Y1 = pChn->nFilter_Y2 = 0;
		pChn->nFilter_Y3 = pChn->nFilter_Y4 = 0;
	}

}

#endif // NO_FILTER
