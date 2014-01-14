/*
 * snd_flt.cpp
 * -----------
 * Purpose: Calculation of resonant filter coefficients.
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Sndfile.h"
#include "Tables.h"

// AWE32: cutoff = reg[0-255] * 31.25 + 100 -> [100Hz-8060Hz]
// EMU10K1 docs: cutoff = reg[0-127]*62+100

#define _USE_MATH_DEFINES
#include <math.h>
#ifndef M_PI
#define M_PI 3.1415926535897932385
#endif


DWORD CSoundFile::CutOffToFrequency(UINT nCutOff, int flt_modifier) const
//-----------------------------------------------------------------------
{
	float Fc;
	ASSERT(nCutOff < 128);
	if(m_SongFlags[SONG_EXFILTERRANGE])
		Fc = 110.0f * pow(2.0f, 0.25f + ((float)(nCutOff * (flt_modifier + 256))) / (20.0f * 512.0f));
	else
		Fc = 110.0f * pow(2.0f, 0.25f + ((float)(nCutOff * (flt_modifier + 256))) / (24.0f * 512.0f));
	LONG freq = (LONG)Fc;
	Limit(freq, 120, 20000);
	if (freq * 2 > (LONG)m_MixerSettings.gdwMixingFreq) freq = m_MixerSettings.gdwMixingFreq >> 1;
	return (DWORD)freq;
}


// Simple 2-poles resonant filter
void CSoundFile::SetupChannelFilter(ModChannel *pChn, bool bReset, int flt_modifier) const
//----------------------------------------------------------------------------------------
{
	int cutoff = (int)pChn->nCutOff + (int)pChn->nCutSwing;
	int resonance = (int)(pChn->nResonance & 0x7F) + (int)pChn->nResSwing;

	Limit(cutoff, 0, 127);
	Limit(resonance, 0, 127);

	if(!GetModFlag(MSF_OLDVOLSWING))
	{
		pChn->nCutOff = (uint8)cutoff;
		pChn->nCutSwing = 0;
		pChn->nResonance = (uint8)resonance;
		pChn->nResSwing = 0;
	}

	float d, e;

	// flt_modifier is in [-256, 256], so cutoff is in [0, 127 * 2] after this calculation.
	const int computedCutoff = cutoff * (flt_modifier + 256) / 256;

	// Filtering is only ever done in IT if either cutoff is not full or if resonance is set.
	if(IsCompatibleMode(TRK_IMPULSETRACKER) && resonance == 0 && computedCutoff >= 254)
	{
		if(pChn->rowCommand.IsNote() && !pChn->dwFlags[CHN_PORTAMENTO] && !pChn->nMasterChn && m_SongFlags[SONG_FIRSTTICK])
		{
			// Z7F next to a note disables the filter, however in other cases this should not happen.
			// Test cases: filter-reset.it, filter-reset-carry.it, filter-nna.it
			pChn->dwFlags.reset(CHN_FILTER);
		}
		return;
	}

	pChn->dwFlags.set(CHN_FILTER);

	if(UseITFilterMode())
	{
		const float freqParameterMultiplier = 128.0f / (24.0f * 256.0f);

		// 2 ^ (i / 24 * 256)
		float frequency = 110.0f * pow(2.0f, 0.25f + (float)computedCutoff * freqParameterMultiplier);
		LimitMax(frequency, (float)(m_MixerSettings.gdwMixingFreq / 2));
		const float r = (float)m_MixerSettings.gdwMixingFreq / (2.0f * (float)M_PI * frequency);

		d = ITResonanceTable[resonance] * r + ITResonanceTable[resonance] - 1.0f;
		e = r * r;
	} else
	{
		float fc = (float)CutOffToFrequency(cutoff, flt_modifier);
		const float dmpfac = pow(10.0f, -((24.0f / 128.0f) * (float)resonance) / 20.0f);

		fc *= (float)(2.0f * (float)M_PI / (float)m_MixerSettings.gdwMixingFreq);

		d = (1.0f - 2.0f * dmpfac) * fc;
		LimitMax(d, 2.0f);
		d = (2.0f * dmpfac - d) / fc;
		e = pow(1.0f / fc, 2.0f);
	}

	float fg = 1.0f / (1.0f + d + e);
	float fb0 = (d + e + e) / (1 + d + e);
	float fb1 = -e / (1.0f + d + e);

#if defined(MPT_INTMIXER)
#define FILTER_CONVERT(x) static_cast<mixsample_t>((x) * (1 << MIXING_FILTER_PRECISION))
#else
#define FILTER_CONVERT(x) (x)
#endif

	switch(pChn->nFilterMode)
	{
	case FLTMODE_HIGHPASS:
		pChn->nFilter_A0 = FILTER_CONVERT(1.0f - fg);
		pChn->nFilter_B0 = FILTER_CONVERT(fb0);
		pChn->nFilter_B1 = FILTER_CONVERT(fb1);
#ifdef MPT_INTMIXER
		pChn->nFilter_HP = -1;
#else
		pChn->nFilter_HP = 1.0f;
#endif // MPT_INTMIXER
		break;

	default:
		pChn->nFilter_A0 = FILTER_CONVERT(fg);
		pChn->nFilter_B0 = FILTER_CONVERT(fb0);
		pChn->nFilter_B1 = FILTER_CONVERT(fb1);
#ifdef MPT_INTMIXER
		pChn->nFilter_HP = 0;
#else
		pChn->nFilter_HP = 0;
#endif // MPT_INTMIXER
		break;
	}
#undef FILTER_CONVERT

	if (bReset)
	{
		pChn->nFilter_Y[0][0] = pChn->nFilter_Y[0][1] = 0;
		pChn->nFilter_Y[1][0] = pChn->nFilter_Y[1][1] = 0;
	}

}
