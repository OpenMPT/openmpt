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
// EMU10K1 docs: cutoff = reg[0-127]*62+100
#define FILTER_PRECISION	8192

#ifndef NO_FILTER

//not gcc friendly...
//#define _ASM_MATH

#ifdef _ASM_MATH

// pow(a,b) returns a^^b -> 2^^(b.log2(a))
__inline float pow(float a, float b)
{
	long tmpint;
	float result;
	_asm {
	fld b				// Load b
	fld a				// Load a
	fyl2x				// ST(0) = b.log2(a)
	fist tmpint			// Store integer exponent
	fisub tmpint		// ST(0) = -1 <= (b*log2(a)) <= 1
	f2xm1				// ST(0) = 2^(x)-1
	fild tmpint			// load integer exponent
	fld1				// Load 1
	fscale				// ST(0) = 2^ST(1)
	fstp ST(1)			// Remove the integer from the stack
	fmul ST(1), ST(0)	// multiply with fractional part
	faddp ST(1), ST(0)	// add integer_part
	fstp result			// Store the result
	}
	return result;
}


#else

#include <math.h>

#endif // _ASM_MATH


extern DWORD LinearSlideUpTable[256]; // 65536 * 2^(x/192)
extern DWORD LinearSlideDownTable[256]; // 65536 * 2^(-x/192)

DWORD CSoundFile::CutOffToFrequency(UINT nCutOff, int flt_modifier)
//-----------------------------------------------------------------
{
#if 1
	float Fc = 110.0f * pow(2.0f, 0.25f + ((float)(nCutOff*(flt_modifier+256)))/(24.0f*512.0f));
	LONG freq = (LONG)Fc;
#else
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
#endif
	if (freq < 120) return 120;
	if (freq > 10000) return 10000;
	return (DWORD)freq;
}


// Simple 2-poles resonant filter
void CSoundFile::SetupChannelFilter(MODCHANNEL *pChn, BOOL bReset, int flt_modifier) const
//----------------------------------------------------------------------------------------
{
	float fc = (float)CutOffToFrequency(pChn->nCutOff, flt_modifier); // [0-255] => [100Hz-8000Hz]
	float fs = (float)gdwMixingFreq;
	float fg, fb0, fb1;

	fc *= 2.0*3.14159265358/fs;
	float dmpfac = pow(10, -((24.0 / 128.0)*(float)pChn->nResonance) / 20.0);
	float d = (1.0-2.0*dmpfac)* fc;
	if (d>2.0) d = 2.0;
	d = (2.0*dmpfac - d)/fc;
	float e = pow(1.0f/fc,2.0);

	fg=1/(1+d+e);
	fb0=(d+e+e)/(1+d+e);
	fb1=-e/(1+d+e);

	pChn->nFilter_B0 = (int)(fg * FILTER_PRECISION);
	pChn->nFilter_B1 = (int)(fb0 * FILTER_PRECISION);
	pChn->nFilter_B2 = (int)(fb1 * FILTER_PRECISION);
	
	if (bReset)
	{
		pChn->nFilter_X1 = pChn->nFilter_X2 = 0;
	}
	pChn->dwFlags |= CHN_FILTER;
}

#endif // NO_FILTER

