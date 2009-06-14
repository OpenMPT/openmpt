/*
 * MODINSTRUMENT related functions.
 */

#include "stdafx.h"
#include "modsmp_ctrl.h"
#include "mptrack/MainFrm.h"

#define new DEBUG_NEW

namespace ctrlSmp
{

void ReplaceSample(MODINSTRUMENT& smp, const LPSTR pNewSample, const SmpLength nNewLength)
//----------------------------------------------------------------------------------------
{
	LPSTR const pOldSmp = smp.pSample;
	BEGIN_CRITICAL();
		smp.pSample = pNewSample;
		smp.nLength = nNewLength;
	END_CRITICAL();
	CSoundFile::FreeSample(pOldSmp);
}


SmpLength InsertSilence(MODINSTRUMENT& smp, const SmpLength nSilenceLength,  const SmpLength nStartFrom, CSoundFile* pSndFile)
//----------------------------------------------------------------------------------------------------------------------------
{
	if(nSilenceLength == 0 || nSilenceLength >= MAX_SAMPLE_LENGTH || smp.nLength > MAX_SAMPLE_LENGTH - nSilenceLength)
		return smp.nLength;

	const SmpLength nOldBytes = smp.GetSampleSizeInBytes();
	const SmpLength nSilenceBytes = nSilenceLength * smp.GetElementarySampleSize() * smp.GetNumChannels();
	const SmpLength nNewSmpBytes = nOldBytes + nSilenceBytes;
	const SmpLength nNewLength = smp.nLength + nSilenceLength;

	LPSTR pNewSmp = 0;
	if( GetSampleCapacity(smp) >= nNewSmpBytes ) // If sample has room to expand.
	{
		AfxMessageBox("Not implemented: GetSampleCapacity(smp) >= nNewSmpBytes");
		// Not implemented, GetSampleCapacity() currently always returns length based value
		// even if there is unused space in the sample.
	}
	else // Have to allocate new sample.
	{
		pNewSmp = CSoundFile::AllocateSample(nNewSmpBytes);
		if(pNewSmp == 0) 
			return smp.nLength; //Sample allocation failed.
		if(nStartFrom == 0)
		{
			memset(pNewSmp, 0, nSilenceBytes);
			memcpy(pNewSmp + nSilenceBytes, smp.pSample, nOldBytes);
		}
		else if(nStartFrom == smp.nLength)
		{
			memcpy(pNewSmp, smp.pSample, nOldBytes);
			memset(pNewSmp + nOldBytes, 0, nSilenceBytes);
		}
		else
			AfxMessageBox(TEXT("Unsupported start position in InsertSilence."));
	}

	ReplaceSample(smp, pNewSmp, nNewLength);
	AdjustEndOfSample(smp, pSndFile);

	return smp.nLength;
}

namespace // Unnamed namespace for local implementation functions.
{

template <class T>
void AdjustEndOfSampleImpl(MODINSTRUMENT& smp)
//--------------------------------------------
{
	MODINSTRUMENT* const pins = &smp;
	const UINT len = pins->nLength;
	T* p = reinterpret_cast<T*>(pins->pSample);
	if (pins->uFlags & CHN_STEREO)
	{
		p[(len+3)*2] = p[(len+2)*2] = p[(len+1)*2] = p[(len)*2] = p[(len-1)*2];
		p[(len+3)*2+1] = p[(len+2)*2+1] = p[(len+1)*2+1] = p[(len)*2+1] = p[(len-1)*2+1];
	} else
	{
		p[len+4] = p[len+3] = p[len+2] = p[len+1] = p[len] = p[len-1];
	}
	if (((pins->uFlags & (CHN_LOOP|CHN_PINGPONGLOOP|CHN_STEREO)) == CHN_LOOP)
	 && (pins->nLoopEnd == pins->nLength)
	 && (pins->nLoopEnd > pins->nLoopStart) && (pins->nLength > 2))
	{
		p[len] = p[pins->nLoopStart];
		p[len+1] = p[pins->nLoopStart+1];
		p[len+2] = p[pins->nLoopStart+2];
		p[len+3] = p[pins->nLoopStart+3];
		p[len+4] = p[pins->nLoopStart+4];
	}
}

} // unnamed namespace.


bool AdjustEndOfSample(MODINSTRUMENT& smp, CSoundFile* pSndFile)
//--------------------------------------------------------------
{
	MODINSTRUMENT* const pins = &smp;

	if ((!pins->nLength) || (!pins->pSample)) 
		return false;

	BEGIN_CRITICAL();

	if (pins->GetElementarySampleSize() == 2)
		AdjustEndOfSampleImpl<int16>(*pins);
	else if(pins->GetElementarySampleSize() == 1)
		AdjustEndOfSampleImpl<int8>(*pins);

	// Update channels with new loop values
	if(pSndFile != 0)
	{
		CSoundFile& rSndFile = *pSndFile;
		for (UINT i=0; i<MAX_CHANNELS; i++) if ((rSndFile.Chn[i].pInstrument == pins) && (rSndFile.Chn[i].nLength))
		{
			if ((pins->nLoopStart + 3 < pins->nLoopEnd) && (pins->nLoopEnd <= pins->nLength))
			{
				rSndFile.Chn[i].nLoopStart = pins->nLoopStart;
				rSndFile.Chn[i].nLoopEnd = pins->nLoopEnd;
				rSndFile.Chn[i].nLength = pins->nLoopEnd;
				if (rSndFile.Chn[i].nPos > rSndFile.Chn[i].nLength)
				{
					rSndFile.Chn[i].nPos = rSndFile.Chn[i].nLoopStart;
					rSndFile.Chn[i].dwFlags &= ~CHN_PINGPONGFLAG;
				}
				DWORD d = rSndFile.Chn[i].dwFlags & ~(CHN_PINGPONGLOOP|CHN_LOOP);
				if (pins->uFlags & CHN_LOOP)
				{
					d |= CHN_LOOP;
					if (pins->uFlags & CHN_PINGPONGLOOP) d |= CHN_PINGPONGLOOP;
				}
				rSndFile.Chn[i].dwFlags = d;
			} else
			if (!(pins->uFlags & CHN_LOOP))
			{
				rSndFile.Chn[i].dwFlags &= ~(CHN_PINGPONGLOOP|CHN_LOOP);
			}
		}
	}
	END_CRITICAL();
	return true;
}


} // namespace ctrlSmp
