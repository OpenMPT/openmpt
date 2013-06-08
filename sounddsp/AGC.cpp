/*
 * AGC.cpp
 * -------
 * Purpose: Automatic Gain Control
 * Notes  : Ugh... This should really be removed at some point.
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "../soundlib/Sndfile.h"
#include "../sounddsp/DSP.h"


//////////////////////////////////////////////////////////////////////////////////
// Automatic Gain Control

#ifndef NO_AGC

#define AGC_PRECISION		10
#define AGC_UNITY			(1 << AGC_PRECISION)

// Limiter
#define MIXING_LIMITMAX		(0x08100000)
#define MIXING_LIMITMIN		(-MIXING_LIMITMAX)


static UINT AGC(int *pBuffer, UINT nSamples, UINT nAGC)
//-----------------------------------------------------
{
	while(nSamples--)
	{
		int val = (int)(((int64)*pBuffer * (int32)nAGC) >> AGC_PRECISION);
		if(val < MIXING_LIMITMIN || val > MIXING_LIMITMAX) nAGC--;
		*pBuffer = val;
		pBuffer++;
	}
	return nAGC;
}


CAGC::CAGC() 
//----------
{
	m_nAGC = AGC_UNITY;
}


void CAGC::Process(int * MixSoundBuffer, int count, DWORD MixingFreq, UINT nChannels)
//-----------------------------------------------------------------------------------
{
	static DWORD gAGCRecoverCount = 0;
	UINT agc = AGC(MixSoundBuffer, count, m_nAGC);
	// Some kind custom law, so that the AGC stays quite stable, but slowly
	// goes back up if the sound level stays below a level inversely proportional
	// to the AGC level. (J'me comprends)
	if ((agc >= m_nAGC) && (m_nAGC < AGC_UNITY))
	{
		gAGCRecoverCount += count;
		UINT agctimeout = MixingFreq >> (AGC_PRECISION-8);
		if (nChannels < 2) agctimeout >>= 1;
		if (gAGCRecoverCount >= agctimeout)
		{
			gAGCRecoverCount = 0;
			m_nAGC++;
		}
	} else
	{
		m_nAGC = agc;
		gAGCRecoverCount = 0;
	}
}


void CAGC::Adjust(UINT oldVol, UINT newVol)
//-----------------------------------------
{
	m_nAGC = m_nAGC * oldVol / newVol;
	if (m_nAGC > AGC_UNITY) m_nAGC = AGC_UNITY;
}


void CAGC::Reset()
//----------------
{
	m_nAGC = AGC_UNITY;
}


#endif // NO_AGC
