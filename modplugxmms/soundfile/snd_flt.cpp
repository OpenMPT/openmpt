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

// AWE32: cutoff = reg[0-255] * 31.25 + 100 -> [100Hz-8060Hz]
#define FILTER_PRECISION	16384

#ifndef NO_FILTER

extern DWORD LinearSlideUpTable[256]; // 65536 * 2^(x/192)
extern DWORD LinearSlideDownTable[256]; // 65536 * 2^(-x/192)

DWORD CSoundFile::CutOffToFrequency(UINT nCutOff, int flt_modifier)
//-----------------------------------------------------------------
{
	LONG freq, fc_exp, fc_mod;

	fc_exp = (((nCutOff * 73) >> 3) * (flt_modifier + 256)); // 140Hz * 2^(cutoff*9/192)
	fc_mod = fc_exp & 0x01FF;
	fc_exp >>= 9;
	freq = MulDiv(125 << (fc_exp / 192), LinearSlideUpTable[fc_exp % 192], 65536);
	// Linear interpolate for finer granularity
	if (fc_mod)
	{
		LONG freq1 = MulDiv(125 << ((fc_exp+1) / 192), LinearSlideUpTable[(fc_exp+1) % 192], 65536);
		freq += ((freq1 - freq) * fc_mod) >> 9;
	}
	if (freq < 120) return 120;
	if (freq > 10000) return 10000;
	return (DWORD)freq;
}


void CSoundFile::SetupChannelFilter(MODCHANNEL *pChn, BOOL bReset, int flt_modifier) const
//----------------------------------------------------------------------------------------
{
	float coef[4];
	//float a0, a1, a2;
	float b0, b1, b2;
	float Q, fc, fs, wp;

	Q = (float)1.0 + ((float)pChn->nResonance) / (float)10.0;		// Resonance
	fc = (float)CutOffToFrequency(pChn->nCutOff, flt_modifier); // [0-255] => [100Hz-8000Hz]
	fs = (float)gdwMixingFreq;	// Sampling frequency

	// Compute z-domain coefficients
	//a0 = 1;
	//a1 = 0;
	//a2 = 0;
	b0 = 1;
	b1 = (float)1.4142 / Q;	// Divide by resonance or Q
	b2 = 1;

	// Transform from s to z domain using bilinear transform with prewarp.
#if 0
	float pi = 4.0 * atan(1.0);
	wp = 2.0 * fs * tan(pi * fc / fs);
#else
	float tmp = fc / fs;
	wp = (float)2.0 * fs * (tmp * (float)3.141592654 + (tmp*tmp*tmp*tmp));
#endif
	b2 /= (wp * wp);
	b1 /= wp;
	// Transform the numerator and denominator coefficients of s-domain biquad 
	// section into corresponding z-domain coefficients.
    //float ad = (float)4.0 * a2 * fs * fs + (float)2.0 * a1 * fs + a0;	// alpha (Numerator in s-domain)
    float bd = (float)4.0 * b2 * fs * fs + (float)2.0 * b1 * fs + b0;	// beta (Denominator in s-domain)
	// Denominator
    coef[0] = - ((float)2.0 * b0 - (float)8.0 * b2 * fs * fs) / bd;			// beta1
    coef[1] = - ((float)4.0 * b2 * fs * fs - (float)2.0 * b1 * fs + b0) / bd;	// beta2
	// Nominator
	//coef[2] = 2; //((float)2.0 * a0 - (float)8.0 * a2 * fs * fs) / ad;			// alpha1
	//coef[3] = 1; //((float)4.0 * a2 * fs * fs - (float)2.0 * a1 * fs + a0) / ad;	// alpha2
	// gain constant
	// k = ad / bd;
	int k = (int)(((float)FILTER_PRECISION) / bd);

	pChn->nFilter_B0 = (k) ? k : 1;
	pChn->nFilter_B1 = (int)(coef[0] * FILTER_PRECISION);
	pChn->nFilter_B2 = (int)(coef[1] * FILTER_PRECISION);
	//pChn->nFilter_A1 = (int)(coef[2] * FILTER_PRECISION);
	//pChn->nFilter_A2 = (int)(coef[3] * FILTER_PRECISION);

	if (bReset)
	{
		pChn->nFilter_X1 = pChn->nFilter_X2 = 0;
	}
	pChn->dwFlags |= CHN_FILTER;
}

#endif // NO_FILTER

