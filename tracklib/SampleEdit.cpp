/*
 * SamplEdit.cpp
 * -------------
 * Purpose: Basic sample editing code (resizing, adding silence, normalizing, ...).
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "SampleEdit.h"
#include "../soundlib/AudioCriticalSection.h"
#include "../soundlib/Sndfile.h"
#include "../soundlib/modsmp_ctrl.h"
#include "../soundbase/SampleFormatConverters.h"
#include "../soundbase/SampleFormatCopy.h"

OPENMPT_NAMESPACE_BEGIN

namespace SampleEdit
{

std::vector<std::reference_wrapper<SmpLength>> GetCuesAndLoops(ModSample &smp)
{
	std::vector<std::reference_wrapper<SmpLength>> loopPoints = {smp.nLoopStart, smp.nLoopEnd, smp.nSustainStart, smp.nSustainEnd};
	loopPoints.insert(loopPoints.end(), std::begin(smp.cues), std::end(smp.cues));
	return loopPoints;
}


SmpLength InsertSilence(ModSample &smp, const SmpLength silenceLength, const SmpLength startFrom, CSoundFile &sndFile)
{
	if(silenceLength == 0 || silenceLength > MAX_SAMPLE_LENGTH || smp.nLength > MAX_SAMPLE_LENGTH - silenceLength || startFrom > smp.nLength)
		return smp.nLength;

	const bool wasEmpty = !smp.HasSampleData();
	const SmpLength newLength = smp.nLength + silenceLength;

	char *pNewSmp = static_cast<char *>(ModSample::AllocateSample(newLength, smp.GetBytesPerSample()));
	if(pNewSmp == nullptr)
		return smp.nLength; //Sample allocation failed.

	if(!wasEmpty)
	{
		// Copy over old sample
		const SmpLength silenceOffset = startFrom * smp.GetBytesPerSample();
		const SmpLength silenceBytes = silenceLength * smp.GetBytesPerSample();
		if(startFrom > 0)
		{
			memcpy(pNewSmp, smp.samplev(), silenceOffset);
		}
		if(startFrom < smp.nLength)
		{
			memcpy(pNewSmp + silenceOffset + silenceBytes, smp.sampleb() + silenceOffset, smp.GetSampleSizeInBytes() - silenceOffset);
		}

		// Update loop points if necessary.
		for(SmpLength &point : GetCuesAndLoops(smp))
		{
			if(point >= startFrom) point += silenceLength;
		}
	} else
	{
		// Set loop points automatically
		smp.nLoopStart = 0;
		smp.nLoopEnd = newLength;
		smp.uFlags.set(CHN_LOOP);
	}

	ctrlSmp::ReplaceSample(smp, pNewSmp, newLength, sndFile);
	smp.PrecomputeLoops(sndFile, true);

	return smp.nLength;
}


namespace
{
	// Update loop points and cues after deleting a sample selection
	void AdjustLoopPoints(SmpLength selStart, SmpLength selEnd, SmpLength &loopStart, SmpLength &loopEnd, SmpLength length)
	{
		Util::DeleteRange(selStart, selEnd - 1, loopStart, loopEnd);

		LimitMax(loopEnd, length);
		if(loopStart + 2 >= loopEnd)
		{
			loopStart = loopEnd = 0;
		}
	}
}

SmpLength RemoveRange(ModSample &smp, SmpLength selStart, SmpLength selEnd, CSoundFile &sndFile)
{
	LimitMax(selEnd, smp.nLength);
	if(selEnd <= selStart)
	{
		return smp.nLength;
	}
	const uint8 bps = smp.GetBytesPerSample();
	memmove(smp.sampleb() + selStart * bps, smp.sampleb() + selEnd * bps, (smp.nLength - selEnd) * bps);
	smp.nLength -= (selEnd - selStart);

	// Did loops or cue points cover the deleted selection?
	AdjustLoopPoints(selStart, selEnd, smp.nLoopStart, smp.nLoopEnd, smp.nLength);
	AdjustLoopPoints(selStart, selEnd, smp.nSustainStart, smp.nSustainEnd, smp.nLength);

	if(smp.nLoopEnd == 0) smp.uFlags.reset(CHN_LOOP | CHN_PINGPONGLOOP);
	if(smp.nSustainEnd == 0) smp.uFlags.reset(CHN_SUSTAINLOOP | CHN_PINGPONGSUSTAIN);

	for(auto &cue : smp.cues)
	{
		if(cue >= selEnd)
			cue -= (selEnd - selStart);
		else if(cue >= selStart && selStart == 0)
			cue = smp.nLength;
		else if(cue >= selStart)
			cue = selStart;
	}

	smp.PrecomputeLoops(sndFile);
	return smp.nLength;
}


SmpLength ResizeSample(ModSample &smp, const SmpLength newLength, CSoundFile &sndFile)
{
	// Invalid sample size
	if(newLength > MAX_SAMPLE_LENGTH || newLength == smp.nLength)
		return smp.nLength;

	// New sample will be bigger so we'll just use "InsertSilence" as it's already there.
	if(newLength > smp.nLength)
		return InsertSilence(smp, newLength - smp.nLength, smp.nLength, sndFile);

	// Else: Shrink sample

	const SmpLength newSmpBytes = newLength * smp.GetBytesPerSample();

	void *newData = ModSample::AllocateSample(newLength, smp.GetBytesPerSample());
	if(newData == nullptr && newLength > 0)
		return smp.nLength; //Sample allocation failed.

	// Copy over old data and replace sample by the new one
	if(newData != nullptr)
		memcpy(newData, smp.sampleb(), newSmpBytes);
	ctrlSmp::ReplaceSample(smp, newData, newLength, sndFile);

	// Sanitize loops and update loop wrap-around buffers
	smp.PrecomputeLoops(sndFile);

	return smp.nLength;
}


void ResetSamples(CSoundFile &sndFile, ResetFlag resetflag, SAMPLEINDEX minSample, SAMPLEINDEX maxSample)
{
	if(minSample == SAMPLEINDEX_INVALID)
		minSample = 1;
	if(maxSample == SAMPLEINDEX_INVALID)
		maxSample = sndFile.GetNumSamples();
	Limit(minSample, SAMPLEINDEX(1), SAMPLEINDEX(MAX_SAMPLES - 1));
	Limit(maxSample, SAMPLEINDEX(1), SAMPLEINDEX(MAX_SAMPLES - 1));

	if(minSample > maxSample)
		std::swap(minSample, maxSample);

	for(SAMPLEINDEX i = minSample; i <= maxSample; i++)
	{
		ModSample &sample = sndFile.GetSample(i);
		switch(resetflag)
		{
		case SmpResetInit:
			sndFile.m_szNames[i] = "";
			sample.filename = "";
			sample.nC5Speed = 8363;
			[[fallthrough]];
		case SmpResetCompo:
			sample.nPan = 128;
			sample.nGlobalVol = 64;
			sample.nVolume = 256;
			sample.nVibDepth = 0;
			sample.nVibRate = 0;
			sample.nVibSweep = 0;
			sample.nVibType = VIB_SINE;
			sample.uFlags.reset(CHN_PANNING | SMP_NODEFAULTVOLUME);
			break;
		case SmpResetVibrato:
			sample.nVibDepth = 0;
			sample.nVibRate = 0;
			sample.nVibSweep = 0;
			sample.nVibType = VIB_SINE;
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
		double max = 0.0, min = 0.0, offset = 0.0;
	};

	// Returns maximum sample amplitude for given sample type (int8/int16).
	template <class T>
	constexpr double GetMaxAmplitude() {return 1.0 + (std::numeric_limits<T>::max)();}

	// Calculates DC offset and returns struct with DC offset, max and min values.
	// DC offset value is average of [-1.0, 1.0[-normalized offset values.
	template<class T>
	OffsetData CalculateOffset(const T *pStart, const SmpLength length)
	{
		OffsetData offsetVals;
		if(length < 1)
			return offsetVals;

		const double intToFloatScale = 1.0 / GetMaxAmplitude<T>();
		double max = -1, min = 1, sum = 0;

		const T *p = pStart;
		for(SmpLength i = 0; i < length; i++, p++)
		{
			const double val = static_cast<double>(*p) * intToFloatScale;
			sum += val;
			if(val > max) max = val;
			if(val < min) min = val;
		}

		offsetVals.max = max;
		offsetVals.min = min;
		offsetVals.offset = (-sum / (double)(length));
		return offsetVals;
	}

	template <class T>
	void RemoveOffsetAndNormalize(T *pStart, const SmpLength length, const double offset, const double amplify)
	{
		T *p = pStart;
		for(SmpLength i = 0; i < length; i++, p++)
		{
			double var = (*p) * amplify + offset;
			*p = mpt::saturate_round<T>(var);
		}
	}
}


// Remove DC offset
double RemoveDCOffset(ModSample &smp, SmpLength start, SmpLength end, CSoundFile &sndFile)
{
	if(!smp.HasSampleData())
		return 0;

	if(end > smp.nLength) end = smp.nLength;
	if(start > end) start = end;
	if(start == end)
	{
		start = 0;
		end = smp.nLength;
	}

	start *= smp.GetNumChannels();
	end *= smp.GetNumChannels();

	const double maxAmplitude = (smp.GetElementarySampleSize() == 2) ? GetMaxAmplitude<int16>() : GetMaxAmplitude<int8>();

	// step 1: Calculate offset.
	OffsetData oData;
	if(smp.GetElementarySampleSize() == 2)
		oData = CalculateOffset(smp.sample16() + start, end - start);
	else if(smp.GetElementarySampleSize() == 1)
		oData = CalculateOffset(smp.sample8() + start, end - start);
	else
		return 0;

	double offset = oData.offset;

	if((int)(offset * maxAmplitude) == 0)
		return 0;

	// those will be changed...
	oData.max += offset;
	oData.min += offset;

	// ... and that might cause distortion, so we will normalize this.
	const double amplify = 1 / std::max(oData.max, -oData.min);

	// step 2: centralize + normalize sample
	offset *= maxAmplitude * amplify;
	if(smp.GetElementarySampleSize() == 2)
		RemoveOffsetAndNormalize(smp.sample16() + start, end - start, offset, amplify);
	else if(smp.GetElementarySampleSize() == 1)
		RemoveOffsetAndNormalize(smp.sample8() + start, end - start, offset, amplify);

	// step 3: adjust global vol (if available)
	if((sndFile.GetType() & (MOD_TYPE_IT | MOD_TYPE_MPT)) && (start == 0) && (end == smp.nLength * smp.GetNumChannels()))
	{
		CriticalSection cs;

		smp.nGlobalVol = std::min(mpt::saturate_round<uint16>(smp.nGlobalVol / amplify), uint16(64));
		for(auto &chn : sndFile.m_PlayState.Chn)
		{
			if(chn.pModSample == &smp)
			{
				chn.UpdateInstrumentVolume(&smp, chn.pModInstrument);
			}
		}
	}

	smp.PrecomputeLoops(sndFile, false);

	return oData.offset;
}


template<typename T>
static void ApplyAmplifyImpl(T * MPT_RESTRICT pSample, const SmpLength length, const double amplifyStart, const double amplifyEnd, const bool isFadeIn, const Fade::Law fadeLaw)
{
	Fade::Func fadeFunc = Fade::GetFadeFunc(fadeLaw);

	if(amplifyStart != amplifyEnd)
	{
		const double fadeOffset = isFadeIn ? amplifyStart : amplifyEnd;
		const double fadeDiff = isFadeIn ? (amplifyEnd - amplifyStart) : (amplifyStart - amplifyEnd);
		const double lengthInv = 1.0 / length;
		for(SmpLength i = 0; i < length; i++)
		{
			const double amp = fadeOffset + fadeFunc(static_cast<double>(isFadeIn ? i : (length - i)) * lengthInv) * fadeDiff;
			pSample[i] = mpt::saturate_round<T>(amp * pSample[i]);
		}
	} else
	{
		const double amp = fadeFunc(amplifyStart);
		for(SmpLength i = 0; i < length; i++)
		{
			pSample[i] = mpt::saturate_round<T>(amp * pSample[i]);
		}
	}
}

bool AmplifySample(ModSample &smp, SmpLength start, SmpLength end, double amplifyStart, double amplifyEnd, bool isFadeIn, Fade::Law fadeLaw, CSoundFile &sndFile)
{
	if(!smp.HasSampleData()) return false;
	if(end == 0 || start >= end || end > smp.nLength)
	{
		start = 0;
		end = smp.nLength;
	}

	if(end - start < 2) return false;

	start *= smp.GetNumChannels();
	end *= smp.GetNumChannels();

	if (smp.GetElementarySampleSize() == 2)
		ApplyAmplifyImpl(smp.sample16() + start, end - start, amplifyStart, amplifyEnd, isFadeIn, fadeLaw);
	else if (smp.GetElementarySampleSize() == 1)
		ApplyAmplifyImpl(smp.sample8() + start, end - start, amplifyStart, amplifyEnd, isFadeIn, fadeLaw);
	else
		return false;

	smp.PrecomputeLoops(sndFile, false);
	return true;
}


template <class T>
static void ReverseSampleImpl(T *pStart, const SmpLength length)
{
	for(SmpLength i = 0; i < length / 2; i++)
	{
		std::swap(pStart[i], pStart[length - 1 - i]);
	}
}

// Reverse sample data
bool ReverseSample(ModSample &smp, SmpLength start, SmpLength end, CSoundFile &sndFile)
{
	if(!smp.HasSampleData()) return false;
	if(end == 0 || start > smp.nLength || end > smp.nLength)
	{
		start = 0;
		end = smp.nLength;
	}

	if(end - start < 2) return false;

	static_assert(MaxSamplingPointSize <= 4);
	if(smp.GetBytesPerSample() == 4)	// 16 bit stereo
		ReverseSampleImpl(static_cast<int32 *>(smp.samplev()) + start, end - start);
	else if(smp.GetBytesPerSample() == 2)	// 16 bit mono / 8 bit stereo
		ReverseSampleImpl(static_cast<int16 *>(smp.samplev()) + start, end - start);
	else if(smp.GetBytesPerSample() == 1)	// 8 bit mono
		ReverseSampleImpl(static_cast<int8 *>(smp.samplev()) + start, end - start);
	else
		return false;

	smp.PrecomputeLoops(sndFile, false);
	return true;
}


template <class T>
static void UnsignSampleImpl(T *pStart, const SmpLength length)
{
	const T offset = (std::numeric_limits<T>::min)();
	for(SmpLength i = 0; i < length; i++)
	{
		pStart[i] += offset;
	}
}

// Virtually unsign sample data
bool UnsignSample(ModSample &smp, SmpLength start, SmpLength end, CSoundFile &sndFile)
{
	if(!smp.HasSampleData()) return false;
	if(end == 0 || start > smp.nLength || end > smp.nLength)
	{
		start = 0;
		end = smp.nLength;
	}
	start *= smp.GetNumChannels();
	end *= smp.GetNumChannels();
	if(smp.GetElementarySampleSize() == 2)
		UnsignSampleImpl(smp.sample16() + start, end - start);
	else if(smp.GetElementarySampleSize() == 1)
		UnsignSampleImpl(smp.sample8() + start, end - start);
	else
		return false;

	smp.PrecomputeLoops(sndFile, false);
	return true;
}


// Invert sample data (flip by 180 degrees)
bool InvertSample(ModSample &smp, SmpLength start, SmpLength end, CSoundFile &sndFile)
{
	return ctrlSmp::InvertSample(smp, start, end, sndFile);
}

// Crossfade sample data to create smooth loops
bool XFadeSample(ModSample &smp, SmpLength fadeLength, int fadeLaw, bool afterloopFade, bool useSustainLoop, CSoundFile &sndFile)
{
	return ctrlSmp::XFadeSample(smp, fadeLength, fadeLaw, afterloopFade, useSustainLoop, sndFile);
}


template <class T>
static void SilenceSampleImpl(T *p, SmpLength length, SmpLength inc, bool fromStart, bool toEnd)
{
	const int dest = toEnd ? 0 : p[(length - 1) * inc];
	const int base = fromStart ? 0 :p[0];
	const int delta = dest - base;
	const int64 len_m1 = length - 1;
	for(SmpLength i = 0; i < length; i++)
	{
		int n = base + static_cast<int>((static_cast<int64>(delta) * static_cast<int64>(i)) / len_m1);
		*p = static_cast<T>(n);
		p += inc;
	}
}

// Silence parts of the sample data
bool SilenceSample(ModSample &smp, SmpLength start, SmpLength end, CSoundFile &sndFile)
{
	LimitMax(end, smp.nLength);
	if(!smp.HasSampleData() || start >= end) return false;

	const SmpLength length = end - start;
	const bool fromStart = start == 0;
	const bool toEnd = end == smp.nLength;
	const uint8 numChn = smp.GetNumChannels();

	for(uint8 chn = 0; chn < numChn; chn++)
	{
		if(smp.GetElementarySampleSize() == 2)
			SilenceSampleImpl(smp.sample16() + start * numChn + chn, length, numChn, fromStart, toEnd);
		else if(smp.GetElementarySampleSize() == 1)
			SilenceSampleImpl(smp.sample8() + start * numChn + chn, length, numChn, fromStart, toEnd);
		else
			return false;
	}

	smp.PrecomputeLoops(sndFile, false);
	return true;
}


template <class T>
static void StereoSepSampleImpl(T *p, SmpLength length, int32 separation)
{
	const int32 fac1 = static_cast<int32>(32768 + separation / 2), fac2 = static_cast<int32>(32768 - separation / 2);
	while(length--)
	{
		const int32 l = p[0], r = p[1];
		p[0] = mpt::saturate_cast<T>((Util::mul32to64(l, fac1) + Util::mul32to64(r, fac2)) >> 16);
		p[1] = mpt::saturate_cast<T>((Util::mul32to64(l, fac2) + Util::mul32to64(r, fac1)) >> 16);
		p += 2;
	}
}

// Change stereo separation
bool StereoSepSample(ModSample &smp, SmpLength start, SmpLength end, double separation, CSoundFile &sndFile)
{
	LimitMax(end, smp.nLength);
	if(!smp.HasSampleData() || start >= end || smp.GetNumChannels() != 2) return false;

	const SmpLength length = end - start;
	const uint8 numChn = smp.GetNumChannels();
	const int32 sep32 = mpt::saturate_round<int32>(separation * (65536.0 / 100.0));

	if(smp.GetElementarySampleSize() == 2)
		StereoSepSampleImpl(smp.sample16() + start * numChn, length, sep32);
	else if(smp.GetElementarySampleSize() == 1)
		StereoSepSampleImpl(smp.sample8() + start * numChn, length, sep32);
	else
		return false;

	smp.PrecomputeLoops(sndFile, false);
	return true;
}


// Convert 16-bit sample to 8-bit
bool ConvertTo8Bit(ModSample &smp, CSoundFile &sndFile)
{
	if(!smp.HasSampleData() || smp.GetElementarySampleSize() != 2)
		return false;

	CopySample<SC::ConversionChain<SC::Convert<int8, int16>, SC::DecodeIdentity<int16>>>(static_cast<int8 *>(smp.samplev()), smp.nLength * smp.GetNumChannels(), 1, smp.sample16(), smp.GetSampleSizeInBytes(), 1);
	smp.uFlags.reset(CHN_16BIT);
	for(auto &chn : sndFile.m_PlayState.Chn)
	{
		if(chn.pModSample == &smp)
			chn.dwFlags.reset(CHN_16BIT);
	}

	smp.PrecomputeLoops(sndFile, false);
	return true;
}


// Convert 8-bit sample to 16-bit
bool ConvertTo16Bit(ModSample &smp, CSoundFile &sndFile)
{
	if(!smp.HasSampleData() || smp.GetElementarySampleSize() != 1)
		return false;

	int16 *newSample = static_cast<int16 *>(ModSample::AllocateSample(smp.nLength, 2 * smp.GetNumChannels()));
	if(newSample == nullptr)
		return false;

	CopySample<SC::ConversionChain<SC::Convert<int16, int8>, SC::DecodeIdentity<int8>>>(newSample, smp.nLength * smp.GetNumChannels(), 1, smp.sample8(), smp.GetSampleSizeInBytes(), 1);
	smp.uFlags.set(CHN_16BIT);
	ctrlSmp::ReplaceSample(smp, newSample, smp.nLength, sndFile);
	smp.PrecomputeLoops(sndFile, false);
	return true;
}


template <class T>
static void ConvertPingPongLoopImpl(T *pStart, SmpLength length)
{
	auto *out = pStart, *in = pStart;
	while(length--)
	{
		*(out++) = *(--in);
	}
}

// Convert ping-pong loops to regular loops
bool ConvertPingPongLoop(ModSample &smp, CSoundFile &sndFile, bool sustainLoop)
{
	if(!smp.HasSampleData()
	   || (!smp.HasPingPongLoop() && !sustainLoop)
	   || (!smp.HasPingPongSustainLoop() && sustainLoop))
		return false;

	const SmpLength loopStart = sustainLoop ? smp.nSustainStart : smp.nLoopStart;
	const SmpLength loopEnd = sustainLoop ? smp.nSustainEnd : smp.nLoopEnd;
	const SmpLength oldLoopLength = loopEnd - loopStart;
	const SmpLength oldLength = smp.nLength;

	if(InsertSilence(smp, oldLoopLength, loopEnd, sndFile) <= oldLength)
		return false;

	static_assert(MaxSamplingPointSize <= 4);
	if(smp.GetBytesPerSample() == 4)  // 16 bit stereo
		ConvertPingPongLoopImpl(static_cast<int32 *>(smp.samplev()) + loopEnd, oldLoopLength);
	else if(smp.GetBytesPerSample() == 2)  // 16 bit mono / 8 bit stereo
		ConvertPingPongLoopImpl(static_cast<int16 *>(smp.samplev()) + loopEnd, oldLoopLength);
	else if(smp.GetBytesPerSample() == 1)  // 8 bit mono
		ConvertPingPongLoopImpl(static_cast<int8 *>(smp.samplev()) + loopEnd, oldLoopLength);
	else
		return false;

	smp.uFlags.reset(sustainLoop ? CHN_PINGPONGSUSTAIN : CHN_PINGPONGLOOP);
	smp.PrecomputeLoops(sndFile, true);
	return true;
}

} // namespace SampleEdit

OPENMPT_NAMESPACE_END
