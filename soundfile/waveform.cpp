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


void X86_Cvt16S_8M(LPBYTE lpSrc, signed char *lpDest, UINT nSamples, DWORD dwInc)
//-------------------------------------------------------------------------------
{
	DWORD dwPos = 0;
	signed char *p = lpDest;
	signed char *s = (signed char *)lpSrc;
	for (UINT i=0; i<nSamples; i++)
	{
		UINT n = dwPos >> 16;
		*p++ = (char)(((int)s[(n << 2)+1] + (int)s[(n << 2)+3]) >> 1);
		dwPos += dwInc;
	}
}


void X86_Cvt8S_8M(LPBYTE lpSrc, signed char *lpDest, UINT nSamples, DWORD dwInc)
//------------------------------------------------------------------------------
{
	DWORD dwPos = 0;
	signed char *p = lpDest;
	unsigned char *s = (unsigned char *)lpSrc;
	for (UINT i=0; i<nSamples; i++)
	{
		UINT n = dwPos >> 16;
		*p++ = (char)((((UINT)s[(n << 1)] + (UINT)s[(n << 1)+1]) >> 1) - 0x80);
		dwPos += dwInc;
	}
}


void X86_Cvt16M_8M(LPBYTE lpSrc, signed char *lpDest, UINT nSamples, DWORD dwInc)
//-------------------------------------------------------------------------------
{
	DWORD dwPos = 0;
	signed char *p = lpDest;
	signed char *s = (signed char *)lpSrc;
	for (UINT i=0; i<nSamples; i++)
	{
		UINT n = dwPos >> 16;
		*p++ = (char)s[(n << 1)+1];
		dwPos += dwInc;
	}
}


void X86_Cvt8M_8M(LPBYTE lpSrc, signed char *lpDest, UINT nSamples, DWORD dwInc)
//------------------------------------------------------------------------------
{
	DWORD dwPos = 0;
	signed char *p = lpDest;
	signed char *s = (signed char *)lpSrc;
	for (UINT i=0; i<nSamples; i++)
	{
		*p++ = (char)(s[dwPos >> 16] - 0x80);
		dwPos += dwInc;
	}
}


void X86_Cvt16S_8S(LPBYTE lpSrc, signed char *lpDest, UINT nSamples, DWORD dwInc)
//-------------------------------------------------------------------------------
{
	DWORD dwPos = 0;
	signed char *p = lpDest;
	signed char *s = (signed char *)lpSrc;
	for (UINT i=0; i<nSamples; i++)
	{
		UINT n = dwPos >> 16;
		p[0] = s[(n << 2)+1];
		p[1] = s[(n << 2)+3];
		p += 2;
		dwPos += dwInc;
	}
}


void X86_Cvt8S_8S(LPBYTE lpSrc, signed char *lpDest, UINT nSamples, DWORD dwInc)
//------------------------------------------------------------------------------
{
	DWORD dwPos = 0;
	signed char *p = lpDest;
	signed char *s = (signed char *)lpSrc;
	for (UINT i=0; i<nSamples; i++)
	{
		UINT n = dwPos >> 16;
		p[0] = (char)(s[n << 1] - 0x80);
		p[1] = (char)(s[(n << 1)+1] - 0x80);
		p += 2;
		dwPos += dwInc;
	}
}


void X86_Cvt16M_8S(LPBYTE lpSrc, signed char *lpDest, UINT nSamples, DWORD dwInc)
//-------------------------------------------------------------------------------
{
	DWORD dwPos = 0;
	signed char *p = lpDest;
	signed char *s = (signed char *)lpSrc;
	for (UINT i=0; i<nSamples; i++)
	{
		UINT n = dwPos >> 16;
		p[1] = p[0] = (char)s[(n << 1)+1];
		p += 2;
		dwPos += dwInc;
	}
}


void X86_Cvt8M_8S(LPBYTE lpSrc, signed char *lpDest, UINT nSamples, DWORD dwInc)
//------------------------------------------------------------------------------
{
	DWORD dwPos = 0;
	signed char *p = lpDest;
	signed char *s = (signed char *)lpSrc;
	for (UINT i=0; i<nSamples; i++)
	{
		p[1] = p[0] = (char)(s[dwPos >> 16] - 0x80);
		p += 2;
		dwPos += dwInc;
	}
}



UINT CSoundFile::WaveConvert(LPBYTE lpSrc, signed char *lpDest, UINT nSamples)
//----------------------------------------------------------------------------
{
	DWORD dwInc;
	if ((!lpSrc) || (!lpDest) || (!nSamples) || (!gdwMixingFreq)) return 0;
	dwInc = MulDiv(gdwMixingFreq, 0x10000, 22050);
	if (gdwSoundSetup & SNDMIX_STEREO)
	{
		if (gnBitsPerSample == 16)
		{
			// Stereo, 16-bit
			X86_Cvt16S_8M(lpSrc, lpDest, nSamples, dwInc);
		} else
		{
			// Stereo, 8-bit
			X86_Cvt8S_8M(lpSrc, lpDest, nSamples, dwInc);
		}
	} else
	{
		if (gnBitsPerSample == 16)
		{
			// Mono, 16-bit
			X86_Cvt16M_8M(lpSrc, lpDest, nSamples, dwInc);
		} else
		{
			// Mono, 8-bit
			X86_Cvt8M_8M(lpSrc, lpDest, nSamples, dwInc);
		}
	}
	return nSamples;
}


UINT CSoundFile::WaveStereoConvert(LPBYTE lpSrc, signed char *lpDest, UINT nSamples)
//----------------------------------------------------------------------------------
{
	DWORD dwInc;
	if ((!lpSrc) || (!lpDest) || (!nSamples) || (!gdwMixingFreq)) return 0;
	dwInc = MulDiv(gdwMixingFreq, 0x10000, 22050);
	if (gdwSoundSetup & SNDMIX_STEREO)
	{
		if (gnBitsPerSample == 16)
		{
			// Stereo, 16-bit
			X86_Cvt16S_8S(lpSrc, lpDest, nSamples, dwInc);
		} else
		{
			// Stereo, 8-bit
			X86_Cvt8S_8S(lpSrc, lpDest, nSamples, dwInc);
		}
	} else
	{
		if (gnBitsPerSample == 16)
		{
			// Mono, 16-bit
			X86_Cvt16M_8S(lpSrc, lpDest, nSamples, dwInc);
		} else
		{
			// Mono, 8-bit
			X86_Cvt8M_8S(lpSrc, lpDest, nSamples, dwInc);
		}
	}
	return nSamples;
}


///////////////////////////////////////////////////////////////////////
// Spectrum Analyzer

extern int SpectrumSinusTable[256*2];


void X86_Spectrum(signed char *pBuffer, UINT nSamples, UINT nInc, UINT nSmpSize, LPLONG lpSinCos)
//-----------------------------------------------------------------------------------------------
{
	signed char *p = pBuffer;
	UINT wt = 0x40;
	LONG ecos = 0, esin = 0;
	for (UINT i=0; i<nSamples; i++)
	{
		int a = *p;
		ecos += a*SpectrumSinusTable[wt & 0x1FF];
		esin += a*SpectrumSinusTable[(wt+0x80) & 0x1FF];
		wt += nInc;
		p += nSmpSize;
	}
	lpSinCos[0] = esin;
	lpSinCos[1] = ecos;
}


LONG CSoundFile::SpectrumAnalyzer(signed char *pBuffer, UINT nSamples, UINT nInc, UINT nChannels)
//-----------------------------------------------------------------------------------------------
{
	LONG sincos[2];
	UINT n = nSamples & (~3);
	if ((pBuffer) && (n))
	{
		sincos[0] = sincos[1] = 0;
		X86_Spectrum(pBuffer, n, nInc, nChannels, sincos);
		LONG esin = sincos[0], ecos = sincos[1];
		if (ecos < 0) ecos = -ecos;
		if (esin < 0) esin = -esin;
		int bug = 64*8 + (64*64 / (nInc+5));
		if (ecos > bug) ecos -= bug; else ecos = 0;
		if (esin > bug) esin -= bug; else esin = 0;
		return ((ecos + esin) << 2) / nSamples;
	}
	return 0;
}



