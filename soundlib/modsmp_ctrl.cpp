/*
 * MODINSTRUMENT related functions.
 */

#include "stdafx.h"
#include "modsmp_ctrl.h"
#include "mptrack/MainFrm.h"

#define new DEBUG_NEW

namespace ctrlSmp
{

void ReplaceSample(MODSAMPLE& smp, const LPSTR pNewSample, const SmpLength nNewLength)
//----------------------------------------------------------------------------------------
{
	LPSTR const pOldSmp = smp.pSample;
	BEGIN_CRITICAL();
		smp.pSample = pNewSample;
		smp.nLength = nNewLength;
	END_CRITICAL();
	CSoundFile::FreeSample(pOldSmp);
}


SmpLength InsertSilence(MODSAMPLE& smp, const SmpLength nSilenceLength, const SmpLength nStartFrom, CSoundFile* pSndFile)
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

SmpLength ResizeSample(MODSAMPLE& smp, const SmpLength nNewLength, CSoundFile* pSndFile)
//----------------------------------------------------------------------------------------
{
	// Invalid sample size
	if(nNewLength > MAX_SAMPLE_LENGTH || nNewLength == smp.nLength)
		return smp.nLength;

	// New sample will be bigger so we'll just use "InsertSilence" as it's already there.
	if(nNewLength > smp.nLength)
		return InsertSilence(smp, nNewLength - smp.nLength, smp.nLength, pSndFile);

	// Else: Shrink sample

	const SmpLength nNewSmpBytes = nNewLength * smp.GetElementarySampleSize() * smp.GetNumChannels();

	LPSTR pNewSmp = 0;
	pNewSmp = CSoundFile::AllocateSample(nNewSmpBytes);
	if(pNewSmp == 0) 
		return smp.nLength; //Sample allocation failed.

	// Copy over old data and replace sample by the new one
	memcpy(pNewSmp, smp.pSample, nNewSmpBytes);
	ReplaceSample(smp, pNewSmp, nNewLength);

	// Adjust loops
	if(smp.nLoopStart > nNewLength)
	{
		smp.nLoopStart = smp.nLoopEnd = 0;
		smp.uFlags &= ~CHN_LOOP;
	}
	if(smp.nLoopEnd > nNewLength) smp.nLoopEnd = nNewLength;
	if(smp.nSustainStart > nNewLength)
	{
		smp.nSustainStart = smp.nSustainEnd = 0;
		smp.uFlags &= ~CHN_SUSTAINLOOP;
	}
	if(smp.nSustainEnd > nNewLength) smp.nSustainEnd = nNewLength;

	AdjustEndOfSample(smp, pSndFile);

	return smp.nLength;
}

namespace // Unnamed namespace for local implementation functions.
{

template <class T>
void AdjustEndOfSampleImpl(MODSAMPLE& smp)
//--------------------------------------------
{
	MODSAMPLE* const pSmp = &smp;
	const UINT len = pSmp->nLength;
	T* p = reinterpret_cast<T*>(pSmp->pSample);
	if (pSmp->uFlags & CHN_STEREO)
	{
		p[(len+3)*2] = p[(len+2)*2] = p[(len+1)*2] = p[(len)*2] = p[(len-1)*2];
		p[(len+3)*2+1] = p[(len+2)*2+1] = p[(len+1)*2+1] = p[(len)*2+1] = p[(len-1)*2+1];
	} else
	{
		p[len+4] = p[len+3] = p[len+2] = p[len+1] = p[len] = p[len-1];
	}
	if (((pSmp->uFlags & (CHN_LOOP|CHN_PINGPONGLOOP|CHN_STEREO)) == CHN_LOOP)
	 && (pSmp->nLoopEnd == pSmp->nLength)
	 && (pSmp->nLoopEnd > pSmp->nLoopStart) && (pSmp->nLength > 2))
	{
		p[len] = p[pSmp->nLoopStart];
		p[len+1] = p[pSmp->nLoopStart+1];
		p[len+2] = p[pSmp->nLoopStart+2];
		p[len+3] = p[pSmp->nLoopStart+3];
		p[len+4] = p[pSmp->nLoopStart+4];
	}
}

} // unnamed namespace.


bool AdjustEndOfSample(MODSAMPLE& smp, CSoundFile* pSndFile)
//--------------------------------------------------------------
{
	MODSAMPLE* const pSmp = &smp;

	if ((!pSmp->nLength) || (!pSmp->pSample)) 
		return false;

	BEGIN_CRITICAL();

	if (pSmp->GetElementarySampleSize() == 2)
		AdjustEndOfSampleImpl<int16>(*pSmp);
	else if(pSmp->GetElementarySampleSize() == 1)
		AdjustEndOfSampleImpl<int8>(*pSmp);

	// Update channels with new loop values
	if(pSndFile != 0)
	{
		CSoundFile& rSndFile = *pSndFile;
		for (UINT i=0; i<MAX_CHANNELS; i++) if ((rSndFile.Chn[i].pModSample == pSmp) && (rSndFile.Chn[i].nLength))
		{
			if ((pSmp->nLoopStart + 3 < pSmp->nLoopEnd) && (pSmp->nLoopEnd <= pSmp->nLength))
			{
				rSndFile.Chn[i].nLoopStart = pSmp->nLoopStart;
				rSndFile.Chn[i].nLoopEnd = pSmp->nLoopEnd;
				rSndFile.Chn[i].nLength = pSmp->nLoopEnd;
				if (rSndFile.Chn[i].nPos > rSndFile.Chn[i].nLength)
				{
					rSndFile.Chn[i].nPos = rSndFile.Chn[i].nLoopStart;
					rSndFile.Chn[i].dwFlags &= ~CHN_PINGPONGFLAG;
				}
				DWORD d = rSndFile.Chn[i].dwFlags & ~(CHN_PINGPONGLOOP|CHN_LOOP);
				if (pSmp->uFlags & CHN_LOOP)
				{
					d |= CHN_LOOP;
					if (pSmp->uFlags & CHN_PINGPONGLOOP) d |= CHN_PINGPONGLOOP;
				}
				rSndFile.Chn[i].dwFlags = d;
			} else
			if (!(pSmp->uFlags & CHN_LOOP))
			{
				rSndFile.Chn[i].dwFlags &= ~(CHN_PINGPONGLOOP|CHN_LOOP);
			}
		}
	}
	END_CRITICAL();
	return true;
}


void ResetSamples(CSoundFile& rSndFile, ResetFlag resetflag)
//----------------------------------------------------------
{
	const UINT nSamples = rSndFile.GetNumSamples();
	for(UINT i = 1; i <= nSamples; i++)
	{
		switch(resetflag)
		{
		case SmpResetInit:
			rSndFile.Samples[i].nC5Speed = 8363;
			// note: break is left out intentionally. keep this order or c&p the stuff from below if you change anything!
		case SmpResetCompo:
			rSndFile.Samples[i].nPan = 128;
			rSndFile.Samples[i].nGlobalVol = 64;
			rSndFile.Samples[i].nVolume = 256;
			rSndFile.Samples[i].nVibDepth = 0;
			rSndFile.Samples[i].nVibRate = 0;
			rSndFile.Samples[i].nVibSweep = 0;
			rSndFile.Samples[i].nVibType = 0;
			rSndFile.Samples[i].uFlags &= ~CHN_PANNING;
			break;
		default:
			break;
		}
	}
}


namespace
{
	struct OffsetData
	{
		double dMax, dMin, dOffset;
	};

	// Returns maximum sample amplitude for given sample type (int8/int16).
	template <class T>
	double GetMaxAmplitude() {return 1.0 + (std::numeric_limits<T>::max)();}

	// Calculates DC offset and returns struct with DC offset, max and min values. 
	// DC offset value is average of [-1.0, 1.0[-normalized offset values.
	template<class T>
	OffsetData CalculateOffset(const T* pStart, const SmpLength nLength)
	//------------------------------------------------------------------
	{
		OffsetData offsetVals = {0,0,0};
		
		if(nLength < 1)
			return offsetVals;

		const double dMaxAmplitude = GetMaxAmplitude<T>();

		double dMax = -1, dMin = 1, dSum = 0;

		const T* p = pStart;
		for (SmpLength i = 0; i < nLength; i++, p++)
		{
			const double dVal = double(*p) / dMaxAmplitude;
			dSum += dVal;
			if(dVal > dMax) dMax = dVal;
			if(dVal < dMin) dMin = dVal;
		}

		offsetVals.dMax = dMax;
		offsetVals.dMin = dMin;
		offsetVals.dOffset = (-dSum / (double)(nLength));
		return offsetVals;
	}

	template <class T>
	void RemoveOffsetAndNormalize(T* pStart, const SmpLength nLength, const double dOffset, const double dAmplify)
	//------------------------------------------------------------------------------------------------------------
	{
		T* p = pStart;
		for (UINT i = 0; i < nLength; i++, p++)
		{
			double dVal = (*p) * dAmplify + dOffset;
			Limit(dVal, (std::numeric_limits<T>::min)(), (std::numeric_limits<T>::max)());
			*p = static_cast<T>(dVal);
		}
	}
};

// Remove DC offset
float RemoveDCOffset(MODSAMPLE& smp,
					 SmpLength iStart,
					 SmpLength iEnd,
					 const MODTYPE modtype,
					 CSoundFile* const pSndFile)
//----------------------------------------------
{
	if(smp.pSample == nullptr || smp.nLength < 1)
		return 0;

	MODSAMPLE* const pSmp = &smp;

	if (iEnd > pSmp->nLength) iEnd = pSmp->nLength;
	if (iStart > iEnd) iStart = iEnd;
	if (iStart == iEnd)
	{
		iStart = 0;
		iEnd = pSmp->nLength;
	}

	iStart *= pSmp->GetNumChannels();
	iEnd *= pSmp->GetNumChannels();

	const double dMaxAmplitude = (pSmp->GetElementarySampleSize() == 2) ? GetMaxAmplitude<int16>() : GetMaxAmplitude<int8>();

	// step 1: Calculate offset.
	OffsetData oData = {0,0,0};
	if(pSmp->GetElementarySampleSize() == 2)
		oData = CalculateOffset(reinterpret_cast<int16*>(pSmp->pSample) + iStart, iEnd - iStart);
	else if(pSmp->GetElementarySampleSize() == 1)
		oData = CalculateOffset(reinterpret_cast<int8*>(pSmp->pSample) + iStart, iEnd - iStart);

	double dMin = oData.dMin, dMax = oData.dMax, dOffset = oData.dOffset;
	
	const float fReportOffset = (float)dOffset;

	if((int)(dOffset * dMaxAmplitude) == 0)
		return 0;
	
	// those will be changed...
	dMax += dOffset;
	dMin += dOffset;

	// ... and that might cause distortion, so we will normalize this.
	const double dAmplify = 1 / max(dMax, -dMin);

	// step 2: centralize + normalize sample
	dOffset *= dMaxAmplitude * dAmplify;
	if(pSmp->GetElementarySampleSize() == 2)
		RemoveOffsetAndNormalize( reinterpret_cast<int16*>(pSmp->pSample) + iStart, iEnd - iStart, dOffset, dAmplify);
	else if(pSmp->GetElementarySampleSize() == 1)
		RemoveOffsetAndNormalize( reinterpret_cast<int8*>(pSmp->pSample) + iStart, iEnd - iStart, dOffset, dAmplify);
	
	// step 3: adjust global vol (if available)
	if((modtype & (MOD_TYPE_IT | MOD_TYPE_MPT)) && (iStart == 0) && (iEnd == pSmp->nLength * pSmp->GetNumChannels()))
		pSmp->nGlobalVol = min((WORD)(pSmp->nGlobalVol / dAmplify), 64);

	AdjustEndOfSample(smp, pSndFile);

	return fReportOffset;
}


} // namespace ctrlSmp
