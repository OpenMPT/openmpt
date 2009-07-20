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


void ResetSamples(CSoundFile& rSndFile, ResetFlag resetflag)
//----------------------------------------------------------
{
	const UINT nSamples = rSndFile.GetNumSamples();
	for(UINT i = 1; i <= nSamples; i++)
	{
		if(resetflag == SmpResetCompo)
		{
			rSndFile.Ins[i].nPan = 128;
			rSndFile.Ins[i].nGlobalVol = 64;
			rSndFile.Ins[i].nVolume = 256;
			rSndFile.Ins[i].nVibDepth = 0;
			rSndFile.Ins[i].nVibRate = 0;
			rSndFile.Ins[i].nVibSweep = 0;
			rSndFile.Ins[i].nVibType = 0;
			rSndFile.Ins[i].uFlags &= ~CHN_PANNING;
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
float RemoveDCOffset(MODINSTRUMENT& smp,
					 SmpLength iStart,
					 SmpLength iEnd,
					 const MODTYPE modtype,
					 CSoundFile* const pSndFile)
//----------------------------------------------
{
	if(smp.pSample == nullptr || smp.nLength < 1)
		return 0;

	MODINSTRUMENT* const pins = &smp;

	if (iEnd > pins->nLength) iEnd = pins->nLength;
	if (iStart > iEnd) iStart = iEnd;
	if (iStart == iEnd)
	{
		iStart = 0;
		iEnd = pins->nLength;
	}

	iStart *= pins->GetNumChannels();
	iEnd *= pins->GetNumChannels();

	const double dMaxAmplitude = (pins->GetElementarySampleSize() == 2) ? GetMaxAmplitude<int16>() : GetMaxAmplitude<int8>();

	// step 1: Calculate offset.
	OffsetData oData = {0,0,0};
	if(pins->GetElementarySampleSize() == 2)
		oData = CalculateOffset(reinterpret_cast<int16*>(pins->pSample) + iStart, iEnd - iStart);
	else if(pins->GetElementarySampleSize() == 1)
		oData = CalculateOffset(reinterpret_cast<int8*>(pins->pSample) + iStart, iEnd - iStart);

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
	if(pins->GetElementarySampleSize() == 2)
		RemoveOffsetAndNormalize( reinterpret_cast<int16*>(pins->pSample) + iStart, iEnd - iStart, dOffset, dAmplify);
	else if(pins->GetElementarySampleSize() == 1)
		RemoveOffsetAndNormalize( reinterpret_cast<int8*>(pins->pSample) + iStart, iEnd - iStart, dOffset, dAmplify);
	
	// step 3: adjust global vol (if available)
	if((modtype & (MOD_TYPE_IT | MOD_TYPE_MPT)) && (iStart == 0) && (iEnd == pins->nLength * pins->GetNumChannels()))
		pins->nGlobalVol = min((WORD)(pins->nGlobalVol / dAmplify), 64);

	AdjustEndOfSample(smp, pSndFile);

	return fReportOffset;
}


} // namespace ctrlSmp
