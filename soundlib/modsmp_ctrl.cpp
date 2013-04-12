/*
 * ModSmp_Ctrl.cpp
 * ---------------
 * Purpose: Basic sample editing code (resizing, adding silence, normalizing, ...).
 * Notes  : Could be merged with ModSample.h / ModSample.cpp at some point.
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "modsmp_ctrl.h"
#include "../common/AudioCriticalSection.h"
#include "../common/Reporting.h"
#include "Sndfile.h"

#define new DEBUG_NEW

namespace ctrlSmp
{

void ReplaceSample(ModSample &smp, void *pNewSample, const SmpLength nNewLength, CSoundFile &sndFile)
//---------------------------------------------------------------------------------------------------
{
	void * const pOldSmp = smp.pSample;
	FlagSet<ChannelFlags> setFlags, resetFlags;

	setFlags.set(CHN_16BIT, smp.uFlags[CHN_16BIT]);
	resetFlags.set(CHN_16BIT, !smp.uFlags[CHN_16BIT]);

	setFlags.set(CHN_STEREO, smp.uFlags[CHN_STEREO]);
	resetFlags.set(CHN_STEREO, !smp.uFlags[CHN_STEREO]);

	CriticalSection cs;

	ctrlChn::ReplaceSample(sndFile.Chn, pOldSmp, pNewSample, nNewLength, setFlags, resetFlags);
	smp.pSample = pNewSample;
	smp.nLength = nNewLength;
	CSoundFile::FreeSample(pOldSmp);
}


SmpLength InsertSilence(ModSample &smp, const SmpLength nSilenceLength, const SmpLength nStartFrom, CSoundFile &sndFile)
//----------------------------------------------------------------------------------------------------------------------
{
	if(nSilenceLength == 0 || nSilenceLength >= MAX_SAMPLE_LENGTH || smp.nLength > MAX_SAMPLE_LENGTH - nSilenceLength)
		return smp.nLength;

	const SmpLength nOldBytes = smp.GetSampleSizeInBytes();
	const SmpLength nSilenceBytes = nSilenceLength * smp.GetBytesPerSample();
	const SmpLength nNewSmpBytes = nOldBytes + nSilenceBytes;
	const SmpLength nNewLength = smp.nLength + nSilenceLength;

	char *pNewSmp = nullptr;
#if 0
	if( GetSampleCapacity(smp) >= nNewSmpBytes ) // If sample has room to expand.
	{
		Reporting::Notification("Not implemented: GetSampleCapacity(smp) >= nNewSmpBytes");
		// Not implemented, GetSampleCapacity() currently always returns length based value
		// even if there is unused space in the sample.
	}
	else // Have to allocate new sample.
#endif
	{
		pNewSmp = static_cast<char *>(CSoundFile::AllocateSample(nNewSmpBytes));
		if(pNewSmp == 0)
			return smp.nLength; //Sample allocation failed.
		if(nStartFrom == 0)
		{
			memcpy(pNewSmp + nSilenceBytes, smp.pSample, nOldBytes);
		}
		else if(nStartFrom == smp.nLength)
		{
			memcpy(pNewSmp, smp.pSample, nOldBytes);
		}
		else
			Reporting::Notification(TEXT("Unsupported start position in InsertSilence."));
	}

	// Set loop points automatically
	if(nOldBytes == 0)
	{
		smp.nLoopStart = 0;
		smp.nLoopEnd = nNewLength;
		smp.uFlags |= CHN_LOOP;
	} else
	{
		if(smp.nLoopStart >= nStartFrom) smp.nLoopStart += nSilenceLength;
		if(smp.nLoopEnd >= nStartFrom) smp.nLoopEnd += nSilenceLength;
	}

	ReplaceSample(smp, pNewSmp, nNewLength, sndFile);
	AdjustEndOfSample(smp, sndFile);

	return smp.nLength;
}


SmpLength ResizeSample(ModSample &smp, const SmpLength nNewLength, CSoundFile &sndFile)
//-------------------------------------------------------------------------------------
{
	// Invalid sample size
	if(nNewLength > MAX_SAMPLE_LENGTH || nNewLength == smp.nLength)
		return smp.nLength;

	// New sample will be bigger so we'll just use "InsertSilence" as it's already there.
	if(nNewLength > smp.nLength)
		return InsertSilence(smp, nNewLength - smp.nLength, smp.nLength, sndFile);

	// Else: Shrink sample

	const SmpLength nNewSmpBytes = nNewLength * smp.GetBytesPerSample();

	void *pNewSmp = nullptr;
	pNewSmp = CSoundFile::AllocateSample(nNewSmpBytes);
	if(pNewSmp == nullptr)
		return smp.nLength; //Sample allocation failed.

	// Copy over old data and replace sample by the new one
	memcpy(pNewSmp, smp.pSample, nNewSmpBytes);
	ReplaceSample(smp, pNewSmp, nNewLength, sndFile);

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

	AdjustEndOfSample(smp, sndFile);

	return smp.nLength;
}

namespace // Unnamed namespace for local implementation functions.
{


template <class T>
void AdjustEndOfSampleImpl(ModSample &smp)
//----------------------------------------
{
	ModSample* const pSmp = &smp;
	const UINT len = pSmp->nLength;
	T *p = static_cast<T *>(pSmp->pSample);
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


bool AdjustEndOfSample(ModSample &smp, CSoundFile &sndFile)
//---------------------------------------------------------
{
	if ((!smp.nLength) || (!smp.pSample))
		return false;

	CriticalSection cs;

	if (smp.GetElementarySampleSize() == 2)
		AdjustEndOfSampleImpl<int16>(smp);
	else if(smp.GetElementarySampleSize() == 1)
		AdjustEndOfSampleImpl<int8>(smp);

	// Update channels with new loop values
	return UpdateLoopPoints(smp, sndFile);

	return true;
}


// Propagate loop point changes to player
bool UpdateLoopPoints(const ModSample &smp, CSoundFile &sndFile)
//--------------------------------------------------------------
{
	if(!smp.nLength || !smp.pSample)
		return false;

	CriticalSection cs;

	// Update channels with new loop values
	for(CHANNELINDEX i = 0; i < MAX_CHANNELS; i++) if((sndFile.Chn[i].pModSample == &smp) && sndFile.Chn[i].nLength != 0)
	{
		bool looped = false, bidi = false;
		ModChannel &chn = sndFile.Chn[i];

		if(smp.nSustainStart < smp.nSustainEnd && smp.nSustainEnd <= smp.nLength && smp.uFlags[CHN_SUSTAINLOOP] && !chn.dwFlags[CHN_KEYOFF])
		{
			// Sustain loop is active
			chn.nLoopStart = smp.nSustainStart;
			chn.nLoopEnd = smp.nSustainEnd;
			chn.nLength = smp.nSustainEnd;
			looped = true;
			bidi = smp.uFlags[CHN_PINGPONGSUSTAIN];
		} else if(smp.nLoopStart < smp.nLoopEnd && smp.nLoopEnd <= smp.nLength && smp.uFlags[CHN_LOOP])
		{
			// Normal loop is active
			chn.nLoopStart = smp.nLoopStart;
			chn.nLoopEnd = smp.nLoopEnd;
			chn.nLength = smp.nLoopEnd;
			looped = true;
			bidi = smp.uFlags[CHN_PINGPONGLOOP];
		}
		chn.dwFlags.set(CHN_LOOP, looped);
		chn.dwFlags.set(CHN_PINGPONGLOOP, looped && bidi);

		if(chn.nPos > chn.nLength)
		{
			chn.nPos = chn.nLoopStart;
			chn.dwFlags.reset(CHN_PINGPONGFLAG);
		}
		
		if(!looped)
		{
			chn.nLength = smp.nLength;
		}
	}

	return true;
}


void ResetSamples(CSoundFile &sndFile, ResetFlag resetflag, SAMPLEINDEX minSample, SAMPLEINDEX maxSample)
//-------------------------------------------------------------------------------------------------------
{
	if(minSample == SAMPLEINDEX_INVALID)
	{
		minSample = 1;
	}
	if(maxSample == SAMPLEINDEX_INVALID)
	{
		maxSample = sndFile.GetNumSamples();
	}
	Limit(minSample, SAMPLEINDEX(1), SAMPLEINDEX(MAX_SAMPLES - 1));
	Limit(maxSample, SAMPLEINDEX(1), SAMPLEINDEX(MAX_SAMPLES - 1));

	if(minSample > maxSample)
	{
		std::swap(minSample, maxSample);
	}

	for(SAMPLEINDEX i = minSample; i <= maxSample; i++)
	{
		ModSample &sample = sndFile.GetSample(i);
		switch(resetflag)
		{
		case SmpResetInit:
			strcpy(sndFile.m_szNames[i], "");
			strcpy(sample.filename, "");
			sample.nC5Speed = 8363;
			// note: break is left out intentionally. keep this order or c&p the stuff from below if you change anything!
		case SmpResetCompo:
			sample.nPan = 128;
			sample.nGlobalVol = 64;
			sample.nVolume = 256;
			sample.nVibDepth = 0;
			sample.nVibRate = 0;
			sample.nVibSweep = 0;
			sample.nVibType = 0;
			sample.uFlags &= ~CHN_PANNING;
			break;
		case SmpResetVibrato:
			sample.nVibDepth = 0;
			sample.nVibRate = 0;
			sample.nVibSweep = 0;
			sample.nVibType = 0;
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
	OffsetData CalculateOffset(const T *pStart, const SmpLength nLength)
	//------------------------------------------------------------------
	{
		OffsetData offsetVals = {0,0,0};

		if(nLength < 1)
			return offsetVals;

		const double dMaxAmplitude = GetMaxAmplitude<T>();

		double dMax = -1, dMin = 1, dSum = 0;

		const T *p = pStart;
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
	void RemoveOffsetAndNormalize(T *pStart, const SmpLength nLength, const double dOffset, const double dAmplify)
	//------------------------------------------------------------------------------------------------------------
	{
		T *p = pStart;
		for (UINT i = 0; i < nLength; i++, p++)
		{
			double dVal = (*p) * dAmplify + dOffset;
			Limit(dVal, (std::numeric_limits<T>::min)(), (std::numeric_limits<T>::max)());
			*p = static_cast<T>(dVal);
		}
	}
};


// Remove DC offset
float RemoveDCOffset(ModSample &smp,
					 SmpLength iStart,
					 SmpLength iEnd,
					 const MODTYPE modtype,
					 CSoundFile &sndFile)
//---------------------------------------
{
	if(smp.pSample == nullptr || smp.nLength < 1)
		return 0;

	if (iEnd > smp.nLength) iEnd = smp.nLength;
	if (iStart > iEnd) iStart = iEnd;
	if (iStart == iEnd)
	{
		iStart = 0;
		iEnd = smp.nLength;
	}

	iStart *= smp.GetNumChannels();
	iEnd *= smp.GetNumChannels();

	const double dMaxAmplitude = (smp.GetElementarySampleSize() == 2) ? GetMaxAmplitude<int16>() : GetMaxAmplitude<int8>();

	// step 1: Calculate offset.
	OffsetData oData = {0,0,0};
	if(smp.GetElementarySampleSize() == 2)
		oData = CalculateOffset(static_cast<int16 *>(smp.pSample) + iStart, iEnd - iStart);
	else if(smp.GetElementarySampleSize() == 1)
		oData = CalculateOffset(static_cast<int8*>(smp.pSample) + iStart, iEnd - iStart);

	double dMin = oData.dMin, dMax = oData.dMax, dOffset = oData.dOffset;

	const float fReportOffset = (float)dOffset;

	if((int)(dOffset * dMaxAmplitude) == 0)
		return 0;

	// those will be changed...
	dMax += dOffset;
	dMin += dOffset;

	// ... and that might cause distortion, so we will normalize this.
	const double dAmplify = 1 / MAX(dMax, -dMin);

	// step 2: centralize + normalize sample
	dOffset *= dMaxAmplitude * dAmplify;
	if(smp.GetElementarySampleSize() == 2)
		RemoveOffsetAndNormalize( static_cast<int16 *>(smp.pSample) + iStart, iEnd - iStart, dOffset, dAmplify);
	else if(smp.GetElementarySampleSize() == 1)
		RemoveOffsetAndNormalize( static_cast<int8 *>(smp.pSample) + iStart, iEnd - iStart, dOffset, dAmplify);

	// step 3: adjust global vol (if available)
	if((modtype & (MOD_TYPE_IT | MOD_TYPE_MPT)) && (iStart == 0) && (iEnd == smp.nLength * smp.GetNumChannels()))
	{
		CriticalSection cs;

		smp.nGlobalVol = MIN((WORD)(smp.nGlobalVol / dAmplify), 64);
		for (CHANNELINDEX i = 0; i < MAX_CHANNELS; i++)
		{
			if(sndFile.Chn[i].pSample == smp.pSample)
			{
				sndFile.Chn[i].nGlobalVol = smp.nGlobalVol;
			}
		}
	}

	AdjustEndOfSample(smp, sndFile);

	return fReportOffset;
}


template <class T>
void ReverseSampleImpl(T *pStart, const SmpLength nLength)
//--------------------------------------------------------
{
	for(SmpLength i = 0; i < nLength / 2; i++)
	{
		std::swap(pStart[i], pStart[nLength - 1 - i]);
	}
}

// Reverse sample data
bool ReverseSample(ModSample &smp, SmpLength iStart, SmpLength iEnd, CSoundFile &sndFile)
//---------------------------------------------------------------------------------------
{
	if(smp.pSample == nullptr) return false;
	if(iEnd == 0 || iStart > smp.nLength || iEnd > smp.nLength)
	{
		iStart = 0;
		iEnd = smp.nLength;
	}

	if(iEnd - iStart < 2) return false;

	if(smp.GetBytesPerSample() == 8)		// unused (yet)
		ASSERT(false); //ReverseSampleImpl(static_cast<int64 *>(smp.pSample) + iStart, iEnd - iStart);
	else if(smp.GetBytesPerSample() == 4)	// 16 bit stereo
		ReverseSampleImpl(static_cast<int32 *>(smp.pSample) + iStart, iEnd - iStart);
	else if(smp.GetBytesPerSample() == 2)	// 16 bit mono / 8 bit stereo
		ReverseSampleImpl(static_cast<int16 *>(smp.pSample) + iStart, iEnd - iStart);
	else if(smp.GetBytesPerSample() == 1)	// 8 bit mono
		ReverseSampleImpl(static_cast<int8 *>(smp.pSample) + iStart, iEnd - iStart);
	else
		return false;

	AdjustEndOfSample(smp, sndFile);
	return true;
}


template <class T>
void UnsignSampleImpl(T *pStart, const SmpLength nLength)
//-------------------------------------------------------
{
	const T offset = (std::numeric_limits<T>::min)();
	for(SmpLength i = 0; i < nLength; i++)
	{
		pStart[i] += offset;
	}
}

// Virtually unsign sample data
bool UnsignSample(ModSample &smp, SmpLength iStart, SmpLength iEnd, CSoundFile &sndFile)
//--------------------------------------------------------------------------------------
{
	if(smp.pSample == nullptr) return false;
	if(iEnd == 0 || iStart > smp.nLength || iEnd > smp.nLength)
	{
		iStart = 0;
		iEnd = smp.nLength;
	}
	iStart *= smp.GetNumChannels();
	iEnd *= smp.GetNumChannels();
	if(smp.GetElementarySampleSize() == 2)
		UnsignSampleImpl(static_cast<int16 *>(smp.pSample) + iStart, iEnd - iStart);
	else if(smp.GetElementarySampleSize() == 1)
		UnsignSampleImpl(static_cast<int8 *>(smp.pSample) + iStart, iEnd - iStart);
	else
		return false;

	AdjustEndOfSample(smp, sndFile);
	return true;
}


template <class T>
void InvertSampleImpl(T *pStart, const SmpLength nLength)
//-------------------------------------------------------
{
	for(SmpLength i = 0; i < nLength; i++)
	{
		pStart[i] = ~pStart[i];
	}
}

// Invert sample data (flip by 180 degrees)
bool InvertSample(ModSample &smp, SmpLength iStart, SmpLength iEnd, CSoundFile &sndFile)
//--------------------------------------------------------------------------------------
{
	if(smp.pSample == nullptr) return false;
	if(iEnd == 0 || iStart > smp.nLength || iEnd > smp.nLength)
	{
		iStart = 0;
		iEnd = smp.nLength;
	}
	iStart *= smp.GetNumChannels();
	iEnd *= smp.GetNumChannels();
	if(smp.GetElementarySampleSize() == 2)
		InvertSampleImpl(static_cast<int16 *>(smp.pSample) + iStart, iEnd - iStart);
	else if(smp.GetElementarySampleSize() == 1)
		InvertSampleImpl(static_cast<int8 *>(smp.pSample) + iStart, iEnd - iStart);
	else
		return false;

	AdjustEndOfSample(smp, sndFile);
	return true;
}


template <class T>
bool EnableSmartSampleRampingImpl(const T *pSample, const SmpLength smpCount)
//---------------------------------------------------------------------------
{
	const T upperThreshold = (std::numeric_limits<T>::max)() / 2;		// >= 50%
	const T lowerThreshold = (std::numeric_limits<T>::max)() / 40;		// <= 2.5%

	// Count backwards for a weighted mean (first samples have the most significant weight).
	T average = pSample[smpCount - 1];
	for(SmpLength i = smpCount; i > 0; i--)
	{
		T data = pSample[i - 1];
		if(data < 0) data = -data;
		average = (data + average) / 2;
	}
	if(average >= upperThreshold || average <= lowerThreshold) return true;
	return false;
}

// This function detects whether to enable smart sample ramping or not, based on the initial DC offset.
// If the DC offset is very high (>= 50%), we can assume that it is somehow intentional (imagine e.g. a typical square waveform sample).
// On the other side, if the initial DC offset is very low (<= 2.5%), we do not need to apply ramping, so we can retain the punch of properly aligned samples.
// Eventually, ramping will (hopefully) only be performed on "bad" samples.
// The function returns true if ramping should be forced off, false if it can stay enabled.
// TODO: It would be a lot nicer if this would pre-normalize samples.
bool EnableSmartSampleRamping(const ModSample &smp, SmpLength sampleOffset, const CSoundFile &sndFile)
//----------------------------------------------------------------------------------------------------
{
	// First two sample points are supposed to be 0 for unlooped MOD samples, so don't take them into account.
	if(!(smp.uFlags & CHN_LOOP) && (sndFile.GetType() & MOD_TYPE_MOD) && sampleOffset == 0) sampleOffset = 2;

	// Just look at the first four samples, starting from the given offset.
	sampleOffset = Util::Min(sampleOffset, smp.nLength);
	const SmpLength smpCount = Util::Min(4u, smp.nLength - sampleOffset) * smp.GetNumChannels();

	if(smp.pSample == nullptr || smpCount == 0) return false;
	if(smp.GetElementarySampleSize() == 2)
		return EnableSmartSampleRampingImpl(static_cast<int16 *>(smp.pSample) + sampleOffset, smpCount);
	else if(smp.GetElementarySampleSize() == 1)
		return EnableSmartSampleRampingImpl(static_cast<int8 *>(smp.pSample) + sampleOffset, smpCount);
	else
		return false;
}


template <class T>
void XFadeSampleImpl(T *pStart, const SmpLength nOffset, SmpLength nFadeLength)
//-----------------------------------------------------------------------------
{
	for(SmpLength i = 0; i <= nFadeLength; i++)
	{
		double dPercentage = sqrt((double)i / (double)nFadeLength); // linear fades are boring
		pStart[nOffset + i] = (T)(((double)pStart[nOffset + i]) * (1 - dPercentage) + ((double)pStart[i]) * dPercentage);
	}
}

// X-Fade sample data to create smooth loop transitions
bool XFadeSample(ModSample &smp, SmpLength iFadeLength, CSoundFile &sndFile)
//--------------------------------------------------------------------------
{
	if(smp.pSample == nullptr) return false;
	if(smp.nLoopEnd <= smp.nLoopStart || smp.nLoopEnd > smp.nLength) return false;
	if(smp.nLoopStart < iFadeLength) return false;

	SmpLength iStart = smp.nLoopStart - iFadeLength;
	SmpLength iEnd = smp.nLoopEnd - iFadeLength;
	iStart *= smp.GetNumChannels();
	iEnd *= smp.GetNumChannels();
	iFadeLength *= smp.GetNumChannels();

	if(smp.GetElementarySampleSize() == 2)
		XFadeSampleImpl(static_cast<int16 *>(smp.pSample) + iStart, iEnd - iStart, iFadeLength);
	else if(smp.GetElementarySampleSize() == 1)
		XFadeSampleImpl(static_cast<int8 *>(smp.pSample) + iStart, iEnd - iStart, iFadeLength);
	else
		return false;

	AdjustEndOfSample(smp, sndFile);
	return true;
}


template <class T>
void ConvertStereoToMonoMixImpl(T *pDest, const SmpLength length)
//---------------------------------------------------------------
{
	const T *pEnd = pDest + length;
	for(T *pSource = pDest; pDest != pEnd; pDest++, pSource += 2)
	{
		*pDest = (pSource[0] + pSource[1] + 1) >> 1;
	}
}


template <class T>
void ConvertStereoToMonoOneChannelImpl(T *pDest, const SmpLength length)
//----------------------------------------------------------------------
{
	const T *pEnd = pDest + length;
	for(T *pSource = pDest; pDest != pEnd; pDest++, pSource += 2)
	{
		*pDest = *pSource;
	}
}


// Convert a multichannel sample to mono (currently only implemented for stereo)
bool ConvertToMono(ModSample &smp, CSoundFile &sndFile, StereoToMonoMode conversionMode)
//--------------------------------------------------------------------------------------
{
	if(smp.pSample == nullptr || smp.nLength == 0 || smp.GetNumChannels() != 2) return false;

	// Note: Sample is overwritten in-place! Unused data is not deallocated!
	if(conversionMode == mixChannels)
	{
		if(smp.GetElementarySampleSize() == 2)
			ConvertStereoToMonoMixImpl(static_cast<int16 *>(smp.pSample), smp.nLength);
		else if(smp.GetElementarySampleSize() == 1)
			ConvertStereoToMonoMixImpl(static_cast<int8 *>(smp.pSample), smp.nLength);
		else
			return false;
	} else
	{
		if(conversionMode == splitSample)
		{
			conversionMode = onlyLeft;
		}
		if(smp.GetElementarySampleSize() == 2)
			ConvertStereoToMonoOneChannelImpl(static_cast<int16 *>(smp.pSample) + (conversionMode == onlyLeft ? 0 : 1), smp.nLength);
		else if(smp.GetElementarySampleSize() == 1)
			ConvertStereoToMonoOneChannelImpl(static_cast<int8 *>(smp.pSample) + (conversionMode == onlyLeft ? 0 : 1), smp.nLength);
		else
			return false;
	}

	CriticalSection cs;
	smp.uFlags &= ~(CHN_STEREO);
	for (CHANNELINDEX i = 0; i < MAX_CHANNELS; i++)
	{
		if(sndFile.Chn[i].pSample == smp.pSample)
		{
			sndFile.Chn[i].dwFlags.reset(CHN_STEREO);
		}
	}

	AdjustEndOfSample(smp, sndFile);
	return true;
}


} // namespace ctrlSmp



namespace ctrlChn
{

void ReplaceSample( ModChannel (&Chn)[MAX_CHANNELS],
					const void * const pOldSample,
					const void * const pNewSample,
					const SmpLength nNewLength,
					FlagSet<ChannelFlags> setFlags,
					FlagSet<ChannelFlags> resetFlags)
{
	for (CHANNELINDEX i = 0; i < MAX_CHANNELS; i++)
	{
		if (Chn[i].pSample == pOldSample)
		{
			Chn[i].pSample = pNewSample;
			if (Chn[i].pCurrentSample != nullptr)
				Chn[i].pCurrentSample = pNewSample;
			if (Chn[i].nPos > nNewLength)
				Chn[i].nPos = 0;
			if (Chn[i].nLength > 0)
				Chn[i].nLength = nNewLength;
			Chn[i].dwFlags.set(setFlags);
			Chn[i].dwFlags.reset(resetFlags);
		}
	}
}

} // namespace ctrlChn
