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
#include "../soundlib/MixFuncTable.h"
#include "../soundlib/mod_specifications.h"
#include "../soundlib/modsmp_ctrl.h"
#include "../soundlib/SampleCopy.h"
#include "../soundlib/Sndfile.h"
#include "openmpt/soundbase/Copy.hpp"
#include "openmpt/soundbase/SampleConvert.hpp"
#include "openmpt/soundbase/SampleConvertFixedPoint.hpp"
#include "openmpt/soundbase/SampleDecode.hpp"

#if MPT_COMPILER_MSVC
#pragma warning(push)
#pragma warning(disable:6011)
#pragma warning(disable:6031)
#endif // MPT_COMPILER_MSVC
#include "../include/r8brain/CDSPResampler.h"
#if MPT_COMPILER_MSVC
#pragma warning(pop)
#endif // MPT_COMPILER_MSVC

#if defined(MPT_ENABLE_ARCH_INTRINSICS_SSE2)
#if MPT_COMPILER_MSVC
#include <intrin.h>
#endif
#include <emmintrin.h>
#endif

OPENMPT_NAMESPACE_BEGIN

namespace SampleEdit
{

#if defined(MPT_ENABLE_ARCH_INTRINSICS_SSE2)

// SSE2 implementation for min/max finder, packs 8*int16 in a 128-bit XMM register.
// scanlen = How many samples to process on this channel
static void sse2_findminmax16(const void *p, SmpLength scanlen, int channels, int &smin, int &smax)
{
	scanlen *= channels;

	// Put minimum / maximum in 8 packed int16 values
	__m128i minVal = _mm_set1_epi16(static_cast<int16>(smin));
	__m128i maxVal = _mm_set1_epi16(static_cast<int16>(smax));

	SmpLength scanlen8 = scanlen / 8;
	if(scanlen8)
	{
		const __m128i *v = static_cast<const __m128i *>(p);
		p = static_cast<const __m128i *>(p) + scanlen8;

		while(scanlen8--)
		{
			__m128i curVals = _mm_loadu_si128(v++);
			minVal = _mm_min_epi16(minVal, curVals);
			maxVal = _mm_max_epi16(maxVal, curVals);
		}

		// Now we have 8 minima and maxima each, in case of stereo they are interleaved L/R values.
		// Move the upper 4 values to the lower half and compute the minima/maxima of that.
		__m128i minVal2 = _mm_unpackhi_epi64(minVal, minVal);
		__m128i maxVal2 = _mm_unpackhi_epi64(maxVal, maxVal);
		minVal = _mm_min_epi16(minVal, minVal2);
		maxVal = _mm_max_epi16(maxVal, maxVal2);

		// Now we have 4 minima and maxima each, in case of stereo they are interleaved L/R values.
		// Move the upper 2 values to the lower half and compute the minima/maxima of that.
		minVal2 = _mm_shuffle_epi32(minVal, _MM_SHUFFLE(1, 1, 1, 1));
		maxVal2 = _mm_shuffle_epi32(maxVal, _MM_SHUFFLE(1, 1, 1, 1));
		minVal = _mm_min_epi16(minVal, minVal2);
		maxVal = _mm_max_epi16(maxVal, maxVal2);

		if(channels < 2)
		{
			// Mono: Compute the minima/maxima of the both remaining values
			minVal2 = _mm_shufflelo_epi16(minVal, _MM_SHUFFLE(1, 1, 1, 1));
			maxVal2 = _mm_shufflelo_epi16(maxVal, _MM_SHUFFLE(1, 1, 1, 1));
			minVal = _mm_min_epi16(minVal, minVal2);
			maxVal = _mm_max_epi16(maxVal, maxVal2);
		}
	}

	const int16 *p16 = static_cast<const int16 *>(p);
	while(scanlen & 7)
	{
		scanlen -= channels;
		__m128i curVals = _mm_set1_epi16(*p16);
		p16 += channels;
		minVal = _mm_min_epi16(minVal, curVals);
		maxVal = _mm_max_epi16(maxVal, curVals);
	}

	smin = static_cast<int16>(_mm_cvtsi128_si32(minVal));
	smax = static_cast<int16>(_mm_cvtsi128_si32(maxVal));
}


// SSE2 implementation for min/max finder, packs 16*int8 in a 128-bit XMM register.
// scanlen = How many samples to process on this channel
static void sse2_findminmax8(const void *p, SmpLength scanlen, int channels, int &smin, int &smax)
{
	scanlen *= channels;

	// Put minimum / maximum in 16 packed int8 values
	__m128i minVal = _mm_set1_epi8(static_cast<int8>(smin ^ 0x80u));
	__m128i maxVal = _mm_set1_epi8(static_cast<int8>(smax ^ 0x80u));

	// For signed <-> unsigned conversion (_mm_min_epi8/_mm_max_epi8 is SSE4)
	__m128i xorVal = _mm_set1_epi8(0x80u);

	SmpLength scanlen16 = scanlen / 16;
	if(scanlen16)
	{
		const __m128i *v = static_cast<const __m128i *>(p);
		p = static_cast<const __m128i *>(p) + scanlen16;

		while(scanlen16--)
		{
			__m128i curVals = _mm_loadu_si128(v++);
			curVals = _mm_xor_si128(curVals, xorVal);
			minVal = _mm_min_epu8(minVal, curVals);
			maxVal = _mm_max_epu8(maxVal, curVals);
		}

		// Now we have 16 minima and maxima each, in case of stereo they are interleaved L/R values.
		// Move the upper 8 values to the lower half and compute the minima/maxima of that.
		__m128i minVal2 = _mm_unpackhi_epi64(minVal, minVal);
		__m128i maxVal2 = _mm_unpackhi_epi64(maxVal, maxVal);
		minVal = _mm_min_epu8(minVal, minVal2);
		maxVal = _mm_max_epu8(maxVal, maxVal2);

		// Now we have 8 minima and maxima each, in case of stereo they are interleaved L/R values.
		// Move the upper 4 values to the lower half and compute the minima/maxima of that.
		minVal2 = _mm_shuffle_epi32(minVal, _MM_SHUFFLE(1, 1, 1, 1));
		maxVal2 = _mm_shuffle_epi32(maxVal, _MM_SHUFFLE(1, 1, 1, 1));
		minVal = _mm_min_epu8(minVal, minVal2);
		maxVal = _mm_max_epu8(maxVal, maxVal2);

		// Now we have 4 minima and maxima each, in case of stereo they are interleaved L/R values.
		// Move the upper 2 values to the lower half and compute the minima/maxima of that.
		minVal2 = _mm_srai_epi32(minVal, 16);
		maxVal2 = _mm_srai_epi32(maxVal, 16);
		minVal = _mm_min_epu8(minVal, minVal2);
		maxVal = _mm_max_epu8(maxVal, maxVal2);

		if(channels < 2)
		{
			// Mono: Compute the minima/maxima of the both remaining values
			minVal2 = _mm_srai_epi16(minVal, 8);
			maxVal2 = _mm_srai_epi16(maxVal, 8);
			minVal = _mm_min_epu8(minVal, minVal2);
			maxVal = _mm_max_epu8(maxVal, maxVal2);
		}
	}

	const int8 *p8 = static_cast<const int8 *>(p);
	while(scanlen & 15)
	{
		scanlen -= channels;
		__m128i curVals = _mm_set1_epi8((*p8) ^ 0x80u);
		p8 += channels;
		minVal = _mm_min_epu8(minVal, curVals);
		maxVal = _mm_max_epu8(maxVal, curVals);
	}

	smin = static_cast<int8>(_mm_cvtsi128_si32(minVal) ^ 0x80u);
	smax = static_cast<int8>(_mm_cvtsi128_si32(maxVal) ^ 0x80u);
}


#endif


std::pair<int, int> FindMinMax(const int8 *p, SmpLength numSamples, int numChannels)
{
	int minVal = 127;
	int maxVal = -128;
#if defined(MPT_ENABLE_ARCH_INTRINSICS_SSE2)
	if(CPU::HasFeatureSet(CPU::feature::sse2) && CPU::HasModesEnabled(CPU::mode::xmm128sse) && numSamples >= 16)
	{
		sse2_findminmax8(p, numSamples, numChannels, minVal, maxVal);
	} else
#endif
	{
		while(numSamples--)
		{

			int s = *p;
			if(s < minVal) minVal = s;
			if(s > maxVal) maxVal = s;
			p += numChannels;
		}
	}
	return { minVal, maxVal };
}


std::pair<int, int> FindMinMax(const int16 *p, SmpLength numSamples, int numChannels)
{
	int minVal = 32767;
	int maxVal = -32768;
#if defined(MPT_ENABLE_ARCH_INTRINSICS_SSE2)
	if(CPU::HasFeatureSet(CPU::feature::sse2) && CPU::HasModesEnabled(CPU::mode::xmm128sse) && numSamples >= 8)
	{
		sse2_findminmax16(p, numSamples, numChannels, minVal, maxVal);
	} else
#endif
	{
		while(numSamples--)
		{
			int s = *p;
			if(s < minVal) minVal = s;
			if(s > maxVal) maxVal = s;
			p += numChannels;
		}
	}
	return { minVal, maxVal };
}


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

	smp.ReplaceWaveform(pNewSmp, newLength, sndFile);
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
	smp.ReplaceWaveform(newData, newLength, sndFile);

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

	if(smp.GetElementarySampleSize() == 2)
		ApplyAmplifyImpl(smp.sample16() + start, end - start, amplifyStart, amplifyEnd, isFadeIn, fadeLaw);
	else if(smp.GetElementarySampleSize() == 1)
		ApplyAmplifyImpl(smp.sample8() + start, end - start, amplifyStart, amplifyEnd, isFadeIn, fadeLaw);
	else
		return false;

	smp.PrecomputeLoops(sndFile, false);
	return true;
}


template<typename T>
static bool ApplyNormalizeImpl(T *p, SmpLength selStart, SmpLength selEnd)
{
	auto [min, max] = FindMinMax(p + selStart, selEnd - selStart, 1);
	max = std::max(-min, max);
	if(max >= std::numeric_limits<T>::max())
		return false;

	max++;
	for(SmpLength i = selStart; i < selEnd; i++)
	{
		p[i] = static_cast<T>((static_cast<int>(p[i]) << (sizeof(T) * 8 - 1)) / max);
	}
	return true;
}


bool NormalizeSample(ModSample &smp, SmpLength start, SmpLength end, CSoundFile &sndFile)
{
	if(!smp.HasSampleData())
		return false;
	LimitMax(end, smp.nLength);
	LimitMax(start, end);
	if(start >= end)
	{
		start = 0;
		end = smp.nLength;
	}

	start *= smp.GetNumChannels();
	end *= smp.GetNumChannels();

	bool modified = false;
	if(smp.uFlags[CHN_16BIT])
		modified = ApplyNormalizeImpl(smp.sample16(), start, end);
	else
		modified = ApplyNormalizeImpl(smp.sample8(), start, end);

	if(modified)
		smp.PrecomputeLoops(sndFile, false);
	return modified;
}


// Reverse sample data
bool ReverseSample(ModSample &smp, SmpLength start, SmpLength end, CSoundFile &sndFile)
{
	return ctrlSmp::ReverseSample(smp, start, end, sndFile);
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
	smp.ReplaceWaveform(newSample, smp.nLength, sndFile);
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

	const auto [loopStart, loopEnd] = sustainLoop ? smp.GetSustainLoop(): smp.GetLoop();
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


// Resample using given resampling method (SRCMODE_DEFAULT = r8brain).
// Returns end point of resampled data, or 0 on failure.
SmpLength Resample(ModSample &smp, SmpLength start, SmpLength end, uint32 newRate, ResamplingMode mode, CSoundFile &sndFile, bool updatePatternCommands, bool updatePatternNotes, const std::function<void()> &prepareSampleUndoFunc, const std::function<void()> &preparePatternUndoFunc)
{
	if(!smp.HasSampleData() || smp.uFlags[CHN_ADLIB] || start >= end)
		return 0;

	const uint32 oldRate = smp.GetSampleRate(sndFile.GetType());
	if(newRate < 1 || oldRate < 1)
		return 0;
	
	const SmpLength oldLength = smp.nLength;
	const SmpLength selLength = (end - start);
	const SmpLength newSelLength = Util::muldivr_unsigned(selLength, newRate, oldRate);
	const SmpLength newSelEnd = start + newSelLength;
	const SmpLength newTotalLength = smp.nLength - selLength + newSelLength;
	const uint8 numChannels = smp.GetNumChannels();
	const bool partialResample = selLength != smp.nLength;

	if(newTotalLength <= 1)
		return 0;

	void *newSample = ModSample::AllocateSample(newTotalLength, smp.GetBytesPerSample());

	if(newSample == nullptr)
		return 0;
	
	// First, copy parts of the sample that are not affected by partial upsampling
	const SmpLength bps = smp.GetBytesPerSample();
	std::memcpy(newSample, smp.sampleb(), start * bps);
	std::memcpy(static_cast<char *>(newSample) + newSelEnd * bps, smp.sampleb() + end * bps, (smp.nLength - end) * bps);

	if(mode == SRCMODE_DEFAULT)
	{
		// Resample using r8brain
		const SmpLength bufferSize = std::min(std::max(selLength, SmpLength(oldRate)), SmpLength(1024 * 1024));
		std::vector<double> convBuffer(bufferSize);
		r8b::CDSPResampler16 resampler(oldRate, newRate, bufferSize);

		for(uint8 chn = 0; chn < numChannels; chn++)
		{
			if(chn != 0)
				resampler.clear();

			SmpLength readCount = selLength, writeCount = newSelLength;
			SmpLength readOffset = start * numChannels + chn, writeOffset = readOffset;
			SmpLength outLatency = newRate;
			double *outBuffer, lastVal = 0.0;

			{
				// Pre-fill the resampler with the first sampling point.
				// Otherwise, it will assume that all samples before the first sampling point are 0,
				// which can lead to unwanted artefacts (ripples) if the sample doesn't start with a zero crossing.
				double firstVal = 0.0;
				switch(smp.GetElementarySampleSize())
				{
					case 1:
						firstVal = SC::Convert<double, int8>()(smp.sample8()[readOffset]);
						lastVal = SC::Convert<double, int8>()(smp.sample8()[readOffset + selLength - numChannels]);
						break;
					case 2:
						firstVal = SC::Convert<double, int16>()(smp.sample16()[readOffset]);
						lastVal = SC::Convert<double, int16>()(smp.sample16()[readOffset + selLength - numChannels]);
						break;
					default:
						// When higher bit depth is added, feel free to also replace CDSPResampler16 by CDSPResampler24 above.
						MPT_ASSERT_MSG(false, "Bit depth not implemented");
				}

				// 10ms or less would probably be enough, but we will pre-fill the buffer with exactly "oldRate" samples
				// to prevent any further rounding errors when using smaller buffers or when dividing oldRate or newRate.
				uint32 remain = oldRate;
				for(SmpLength i = 0; i < bufferSize; i++)
					convBuffer[i] = firstVal;
				while(remain > 0)
				{
					uint32 procIn = std::min(remain, mpt::saturate_cast<uint32>(bufferSize));
					SmpLength procCount = resampler.process(convBuffer.data(), procIn, outBuffer);
					MPT_ASSERT(procCount <= outLatency);
					LimitMax(procCount, outLatency);
					outLatency -= procCount;
					remain -= procIn;
				}
			}

			// Now we can start with the actual resampling work...
			while(writeCount > 0)
			{
				SmpLength smpCount = (SmpLength)convBuffer.size();
				if(readCount != 0)
				{
					LimitMax(smpCount, readCount);

					switch(smp.GetElementarySampleSize())
					{
						case 1:
							CopySample<SC::ConversionChain<SC::Convert<double, int8>, SC::DecodeIdentity<int8>>>(convBuffer.data(), smpCount, 1, smp.sample8() + readOffset, smp.GetSampleSizeInBytes(), smp.GetNumChannels());
							break;
						case 2:
							CopySample<SC::ConversionChain<SC::Convert<double, int16>, SC::DecodeIdentity<int16>>>(convBuffer.data(), smpCount, 1, smp.sample16() + readOffset, smp.GetSampleSizeInBytes(), smp.GetNumChannels());
							break;
					}
					readOffset += smpCount * numChannels;
					readCount -= smpCount;
				} else
				{
					// Nothing to read, but still to write (compensate for r8brain's output latency)
					for(SmpLength i = 0; i < smpCount; i++)
						convBuffer[i] = lastVal;
				}

				SmpLength procCount = resampler.process(convBuffer.data(), smpCount, outBuffer);
				const SmpLength procLatency = std::min(outLatency, procCount);
				procCount = std::min(procCount - procLatency, writeCount);

				switch(smp.GetElementarySampleSize())
				{
					case 1:
						CopySample<SC::ConversionChain<SC::Convert<int8, double>, SC::DecodeIdentity<double>>>(static_cast<int8 *>(newSample) + writeOffset, procCount, smp.GetNumChannels(), outBuffer + procLatency, procCount * sizeof(double), 1);
						break;
					case 2:
						CopySample<SC::ConversionChain<SC::Convert<int16, double>, SC::DecodeIdentity<double>>>(static_cast<int16 *>(newSample) + writeOffset, procCount, smp.GetNumChannels(), outBuffer + procLatency, procCount * sizeof(double), 1);
						break;
				}
				writeOffset += procCount * numChannels;
				writeCount -= procCount;
				outLatency -= procLatency;
			}
		}
	} else
	{
		// Resample using built-in filters
		uint32 functionNdx = MixFuncTable::ResamplingModeToMixFlags(mode);
		if(smp.uFlags[CHN_16BIT]) functionNdx |= MixFuncTable::ndx16Bit;
		if(smp.uFlags[CHN_STEREO]) functionNdx |= MixFuncTable::ndxStereo;
		ModChannel chn{};
		chn.pCurrentSample = smp.samplev();
		chn.increment = SamplePosition::Ratio(oldRate, newRate);
		//chn.increment = SamplePosition(((static_cast<int64>(oldRate) << 32) + newRate / 2) / newRate);
		chn.position.Set(start);
		chn.leftVol = chn.rightVol = (1 << 8);
		chn.nLength = smp.nLength;

		SmpLength writeCount = newSelLength;
		SmpLength writeOffset = start * smp.GetNumChannels();
		while(writeCount > 0)
		{
			SmpLength procCount = std::min(static_cast<SmpLength>(MIXBUFFERSIZE), writeCount);
			mixsample_t buffer[MIXBUFFERSIZE * 2];
			MemsetZero(buffer);
			MixFuncTable::Functions[functionNdx](chn, sndFile.m_Resampler, buffer, procCount);

			for(uint8 c = 0; c < numChannels; c++)
			{
				switch(smp.GetElementarySampleSize())
				{
					case 1:
						CopySample<SC::ConversionChain<SC::ConvertFixedPoint<int8, mixsample_t, 23>, SC::DecodeIdentity<mixsample_t>>>(static_cast<int8 *>(newSample) + writeOffset + c, procCount, smp.GetNumChannels(), buffer + c, sizeof(buffer), 2);
						break;
					case 2:
						CopySample<SC::ConversionChain<SC::ConvertFixedPoint<int16, mixsample_t, 23>, SC::DecodeIdentity<mixsample_t>>>(static_cast<int16 *>(newSample) + writeOffset + c, procCount, smp.GetNumChannels(), buffer + c, sizeof(buffer), 2);
						break;
				}
			}

			writeCount -= procCount;
			writeOffset += procCount * smp.GetNumChannels();
		}
	}

	prepareSampleUndoFunc();

	// Adjust loops and cues
	const auto oldCues = smp.cues;
	for(SmpLength &point : GetCuesAndLoops(smp))
	{
		if(point >= oldLength)
			point = newTotalLength;
		else if(point >= end)
			point += newSelLength - selLength;
		else if(point > start)
			point = start + Util::muldivr_unsigned(point - start, newRate, oldRate);
		LimitMax(point, newTotalLength);
	}

	const SAMPLEINDEX sampleIndex = static_cast<SAMPLEINDEX>(std::distance(&sndFile.GetSample(0), &smp));
	bool patternUndoCreated = false;
	if(updatePatternCommands)
	{
		sndFile.Patterns.ForEachModCommand([&](ModCommand &m)
		{
			if(m.command != CMD_OFFSET && m.command != CMD_REVERSEOFFSET && m.command != CMD_OFFSETPERCENTAGE)
				return;
			if(sndFile.GetSampleIndex(m.note, m.instr) != sampleIndex)
				return;
			SmpLength point = m.param * 256u;

			if(m.command == CMD_OFFSETPERCENTAGE || (m.volcmd == VOLCMD_OFFSET && m.vol == 0))
				point = Util::muldivr_unsigned(point, oldLength, 65536);
			else if(m.volcmd == VOLCMD_OFFSET && m.vol <= std::size(oldCues))
				point += oldCues[m.vol - 1];

			if(point >= oldLength)
				point = newTotalLength;
			else if(point >= end)
				point += newSelLength - selLength;
			else if(point > start)
				point = start + Util::muldivr_unsigned(point - start, newRate, oldRate);
			LimitMax(point, newTotalLength);

			if(m.command == CMD_OFFSETPERCENTAGE || (m.volcmd == VOLCMD_OFFSET && m.vol == 0))
				point = Util::muldivr_unsigned(point, 65536, newTotalLength);
			else if(m.volcmd == VOLCMD_OFFSET && m.vol <= std::size(smp.cues))
				point -= smp.cues[m.vol - 1];
			if(!patternUndoCreated)
			{
				patternUndoCreated = true;
				preparePatternUndoFunc();
			}
			m.param = mpt::saturate_cast<ModCommand::PARAM>(point / 256u);
		});
	}

	const double transpose = 12.0 * std::log(static_cast<double>(newRate) / oldRate) / std::log(2.0);
	int noteAdjust = mpt::saturate_round<int>(transpose);
	if(updatePatternNotes && noteAdjust != 0)
	{
		sndFile.Patterns.ForEachModCommand([&](ModCommand &m)
		{
			if(!m.IsNote())
				return;
			if(sndFile.GetSampleIndex(m.note, m.instr) != sampleIndex)
				return;
			
			const auto newNote = static_cast<ModCommand::NOTE>(Clamp(m.note + noteAdjust, sndFile.GetModSpecifications().noteMin, sndFile.GetModSpecifications().noteMax));
			if(m.note == newNote)
				return;
			
			if(!patternUndoCreated)
			{
				patternUndoCreated = true;
				preparePatternUndoFunc();
			}
			m.note = newNote;
		});
	}

	if(!partialResample || updatePatternNotes)
	{
		if (sndFile.GetBestSaveFormat() == MOD_TYPE_MOD)
		{
			int finetuneAdjust = smp.nFineTune / 16 + mpt::saturate_round<int>((transpose - noteAdjust) * 8.0);
			if(finetuneAdjust >= 8)
			{
				finetuneAdjust -= 16;
				noteAdjust++;
			} else if(finetuneAdjust < -8)
			{
				finetuneAdjust += 16;
				noteAdjust--;
			}
			smp.nFineTune = MOD2XMFineTune(finetuneAdjust);
		} else
		{
			smp.nC5Speed = newRate;
			smp.FrequencyToTranspose();
		}
	}

	smp.ReplaceWaveform(newSample, newTotalLength, sndFile);
	// Update loop wrap-around buffer
	smp.PrecomputeLoops(sndFile);

	return newSelEnd;
}


static constexpr int SMPLOOP_ACCURACY = 7;  // 5%

static bool LoopCheck(int sstart0, int sstart1, int send0, int send1)
{
	int dse0 = send0 - sstart0;
	if((dse0 < -SMPLOOP_ACCURACY) || (dse0 > SMPLOOP_ACCURACY))
		return false;
	int dse1 = send1 - sstart1;
	if((dse1 < -SMPLOOP_ACCURACY) || (dse1 > SMPLOOP_ACCURACY))
		return false;
	int dstart = sstart1 - sstart0;
	int dend = send1 - send0;
	if(!dstart)
		dstart = dend >> 7;
	if(!dend)
		dend = dstart >> 7;
	if((dstart ^ dend) < 0)
		return false;
	int delta = dend - dstart;
	return ((delta > -SMPLOOP_ACCURACY) && (delta < SMPLOOP_ACCURACY));
}


static bool BidiEndCheck(int spos0, int spos1, int spos2)
{
	int delta0 = spos1 - spos0;
	int delta1 = spos2 - spos1;
	int delta2 = spos2 - spos0;
	if(!delta0)
		delta0 = delta1 >> 7;
	if(!delta1)
		delta1 = delta0 >> 7;
	if((delta1 ^ delta0) < 0)
		return false;
	return ((delta0 >= -1) && (delta0 <= 0) && (delta1 >= -1) && (delta1 <= 0) && (delta2 >= -1) && (delta2 <= 0));
}


static bool BidiStartCheck(int spos0, int spos1, int spos2)
{
	int delta1 = spos1 - spos0;
	int delta0 = spos2 - spos1;
	int delta2 = spos2 - spos0;
	if(!delta0)
		delta0 = delta1 >> 7;
	if(!delta1)
		delta1 = delta0 >> 7;
	if((delta1 ^ delta0) < 0)
		return false;
	return ((delta0 >= -1) && (delta0 <= 0) && (delta1 > -1) && (delta1 <= 0) && (delta2 >= -1) && (delta2 <= 0));
}


SmpLength FindLoopStart(const ModSample &sample, bool sustainLoop, bool goForward, bool moveLoop)
{
	const uint8 *pSample = mpt::byte_cast<const uint8 *>(sample.sampleb());
	if(sample.uFlags[CHN_16BIT] && mpt::endian_is_little())
		pSample++;
	const uint32 inc = sample.GetBytesPerSample();
	const auto [loopStart, loopEnd] = sustainLoop ? sample.GetSustainLoop() : sample.GetLoop();
	const SmpLength loopLength = loopEnd - loopStart;
	const bool pingpong = sample.uFlags[sustainLoop ? CHN_PINGPONGSUSTAIN : CHN_PINGPONGLOOP];

	const uint8 *p = pSample + loopStart * inc;
	int find0 = static_cast<int>(pSample[loopEnd * inc - inc]);
	int find1 = static_cast<int>(pSample[loopEnd * inc]);
	if(goForward)
	{
		// Find Next LoopStart Point
		const SmpLength searchEnd = moveLoop ? sample.nLength - loopLength : (std::max(loopEnd, SmpLength(16)) - 16);
		for(SmpLength i = sample.nLoopStart + 1; i <= searchEnd; i++)
		{
			p += inc;
			if(pingpong)
			{
				if(BidiStartCheck(p[0], p[inc], p[inc * 2]))
					return i;
			} else
			{
				if(moveLoop)
				{
					find0 = static_cast<int>(pSample[(i + loopLength - 1) * inc]);
					find1 = static_cast<int>(pSample[(i + loopLength) * inc]);
				}
				if(LoopCheck(find0, find1, p[0], p[inc]))
					return i;
			}
		}
	} else
	{
		// Find Prev LoopStart Point
		for(SmpLength i = sample.nLoopStart; i;)
		{
			i--;
			p -= inc;
			if(pingpong)
			{
				if(BidiStartCheck(p[0], p[inc], p[inc * 2]))
					return i;
			} else
			{
				if(moveLoop)
				{
					find0 = static_cast<int>(pSample[(i + loopLength - 1) * inc]);
					find1 = static_cast<int>(pSample[(i + loopLength) * inc]);
				}
				if(LoopCheck(find0, find1, p[0], p[inc]))
					return i;
			}
		}
	}
	return sample.nLength;
}


SmpLength FindLoopEnd(const ModSample &sample, bool sustainLoop, bool goForward, bool moveLoop)
{
	const uint8 *pSample = mpt::byte_cast<const uint8 *>(sample.sampleb());
	if(sample.uFlags[CHN_16BIT] && mpt::endian_is_little())
		pSample++;
	const uint32 inc = sample.GetBytesPerSample();
	const auto [loopStart, loopEnd] = sustainLoop ? sample.GetSustainLoop() : sample.GetLoop();
	const SmpLength loopLength = loopEnd - loopStart;
	const bool pingpong = sample.uFlags[sustainLoop ? CHN_PINGPONGSUSTAIN : CHN_PINGPONGLOOP];

	const uint8 *p = pSample + loopEnd * inc;
	int find0 = static_cast<int>(pSample[loopStart * inc]);
	int find1 = static_cast<int>(pSample[loopStart * inc + inc]);
	if(goForward)
	{
		// Find Next LoopEnd Point
		for(SmpLength i = loopEnd + 1; i <= sample.nLength; i++)
		{
			p += inc;
			if(pingpong)
			{
				if(BidiEndCheck(p[0], p[inc], p[inc * 2]))
					return i;
			} else
			{
				if(moveLoop)
				{
					find0 = static_cast<int>(pSample[(i - loopLength) * inc]);
					find1 = static_cast<int>(pSample[(i - loopLength + 1) * inc]);
				}
				if(LoopCheck(find0, find1, p[0], p[inc]))
					return i;
			}
		}
	} else
	{
		// Find Prev LoopEnd Point
		const SmpLength searchEnd = moveLoop ? loopLength : (loopStart + 16);
		for(SmpLength i = loopEnd; i > searchEnd;)
		{
			i--;
			p -= inc;
			if(pingpong)
			{
				if(BidiEndCheck(p[0], p[inc], p[inc * 2]))
					return i;
			} else
			{
				if(moveLoop)
				{
					find0 = static_cast<int>(pSample[(i - loopLength) * inc]);
					find1 = static_cast<int>(pSample[(i - loopLength + 1) * inc]);
				}
				if(LoopCheck(find0, find1, p[0], p[inc]))
					return i;
			}
		}
	}
	return 0;
}


} // namespace SampleEdit

OPENMPT_NAMESPACE_END
