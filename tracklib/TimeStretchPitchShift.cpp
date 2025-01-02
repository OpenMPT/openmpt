/*
 * TimeStretchPitchShift.cpp
 * -------------------------
 * Purpose: Classes for applying different types of offline time stretching and pitch shifting to samples.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "TimeStretchPitchShift.h"
#include "SampleEdit.h"
#include "../soundlib/ModSample.h"
#include "../soundlib/SampleCopy.h"
#include "../soundlib/Sndfile.h"
#include "openmpt/soundbase/SampleConvert.hpp"
#include "openmpt/soundbase/SampleDecode.hpp"

#if MPT_COMPILER_MSVC
#pragma warning(push)
#pragma warning(disable:4305)
#pragma warning(disable:4267)
#pragma warning(disable:4244)
#pragma warning(disable:4458)
#endif  // MPT_COMPILER_MSVC
#include <SignalsmithStretch/signalsmith-stretch.h>
#if MPT_COMPILER_MSVC
#pragma warning(pop)
#endif  // MPT_COMPILER_MSVC


OPENMPT_NAMESPACE_BEGIN

namespace TimeStretchPitchShift
{
Base::Base(UpdateProgressFunc updateProgress, PrepareUndoFunc prepareUndo, CSoundFile &sndFile, SAMPLEINDEX sample, float pitch, float stretchRatio, SmpLength start, SmpLength end)
	: UpdateProgress{std::move(updateProgress)}
	, PrepareUndo{std::move(prepareUndo)}
	, m_sndFile{sndFile}
	, m_sample{m_sndFile.GetSample(sample)}
	, m_pitch{pitch}
	, m_stretchRatio{stretchRatio}
	, m_start{start}
	, m_end{end}
	, m_selLength{m_end - m_start}
	, m_newSelLength{mpt::saturate_round<SmpLength>(m_selLength * m_stretchRatio)}
	, m_newLength{m_sample.nLength - m_selLength + m_newSelLength}
	, m_stretchEnd{m_start + m_newSelLength}
{
}


Result Base::CheckPreconditions() const
{
	if(!m_sample.HasSampleData() || m_start >= m_sample.nLength)
		return Result::Abort;
	if(m_newSelLength < 2)
		return Result::StretchTooShort;
	if(m_newSelLength > MAX_SAMPLE_LENGTH)
		return Result::StretchTooLong;
	if(MAX_SAMPLE_LENGTH - m_newSelLength < m_sample.nLength - m_selLength)
		return Result::StretchTooLong;
	return Result::OK;
}


void Base::FinishProcessing(void *newSampleData)
{
	PrepareUndo();

	// Copy sample data outside of selection
	const auto bps = m_sample.GetBytesPerSample();
	memcpy(newSampleData, m_sample.sampleb(), m_start * bps);
	memcpy(static_cast<std::byte *>(newSampleData) + m_stretchEnd * bps, m_sample.sampleb() + m_end * bps, (m_sample.nLength - m_end) * bps);

	m_sample.ReplaceWaveform(newSampleData, m_newLength, m_sndFile);

	if(m_stretchRatio != 1.0)
	{
		for(SmpLength &cue : SampleEdit::GetCuesAndLoops(m_sample))
		{
			if(cue > m_start)
				cue = m_start + mpt::saturate_round<SmpLength>((cue - m_start) * m_stretchRatio);
		}
	}
	m_sample.SetLoop(
		m_sample.nLoopStart,
		m_sample.nLoopEnd,
		m_sample.uFlags[CHN_LOOP],
		m_sample.uFlags[CHN_PINGPONGLOOP],
		m_sndFile);
	m_sample.SetSustainLoop(
		m_sample.nSustainStart,
		m_sample.nSustainEnd,
		m_sample.uFlags[CHN_SUSTAINLOOP],
		m_sample.uFlags[CHN_PINGPONGSUSTAIN],
		m_sndFile);
}


Result Signalsmith::Process()
{
	if(const auto result = CheckPreconditions(); result != Result::OK)
		return result;

	const auto smpSize = m_sample.GetElementarySampleSize();
	const auto numChans = m_sample.GetNumChannels();

	using ProcessingType = float;
	signalsmith::stretch::SignalsmithStretch<ProcessingType> stretch;
	stretch.presetDefault(numChans, static_cast<ProcessingType>(m_sample.GetSampleRate(m_sndFile.GetType())));
	stretch.setTransposeFactor(m_pitch);

	const auto [inBufferLength, outBufferLength] = CalculateBufferSizes();
	std::vector<ProcessingType> in, out;
	try
	{
		in.resize(inBufferLength * numChans);
		out.resize(outBufferLength * numChans);
	} catch(mpt::out_of_memory e)
	{
		mpt::delete_out_of_memory(e);
		return Result::OutOfMemory;
	}

	void *newSampleData = ModSample::AllocateSample(m_newLength, m_sample.GetBytesPerSample());
	if(newSampleData == nullptr)
		return Result::OutOfMemory;

	std::array<mpt::span<ProcessingType>, 2> inputBuffers, outputBuffers;
	for(uint8 chn = 0; chn < numChans; chn++)
	{
		inputBuffers[chn] = mpt::as_span(in).subspan(chn * inBufferLength, inBufferLength);
		outputBuffers[chn] = mpt::as_span(out).subspan(chn * outBufferLength, outBufferLength);
	}

	SmpLength prerollRemain = mpt::saturate_round<SmpLength>(stretch.outputLatency() + stretch.inputLatency() * m_stretchRatio);
	SmpLength inPos = m_start * numChans, inRemain = m_selLength;
	SmpLength outPos = m_start * numChans, outRemain = m_newSelLength;
	while((inRemain || outRemain))
	{
		if(UpdateProgress(m_newSelLength - outRemain, m_newSelLength))
		{
			ModSample::FreeSample(newSampleData);
			return Result::Abort;
		}

		const SmpLength processInLen = std::min(inRemain, inBufferLength);

		for(uint8 chn = 0; chn < numChans; chn++)
		{
			switch(smpSize)
			{
			case 1:
				CopySample<SC::ConversionChain<SC::Convert<ProcessingType, int8>, SC::DecodeIdentity<int8>>>(inputBuffers[chn].data(), processInLen, 1, m_sample.sample8() + inPos + chn, sizeof(int8) * inRemain * numChans, numChans);
				break;
			case 2:
				CopySample<SC::ConversionChain<SC::Convert<ProcessingType, int16>, SC::DecodeIdentity<int16>>>(inputBuffers[chn].data(), processInLen, 1, m_sample.sample16() + inPos + chn, sizeof(int16) * inRemain * numChans, numChans);
				break;
			}
			// If we're at the end of the sample, fill the remaining buffer with silence.
			std::fill(inputBuffers[chn].begin() + processInLen, inputBuffers[chn].end(), 0.0f);
		}

		stretch.process(inputBuffers, inBufferLength, outputBuffers, outBufferLength);

		const SmpLength readOffset = std::min(prerollRemain, outBufferLength);
		const SmpLength processOutLen = std::min(outRemain, outBufferLength - readOffset);

		for(uint8 chn = 0; chn < numChans; chn++)
		{
			switch(smpSize)
			{
			case 1:
				CopySample<SC::ConversionChain<SC::Convert<int8, ProcessingType>, SC::DecodeIdentity<ProcessingType>>>(static_cast<int8 *>(newSampleData) + outPos + chn, processOutLen, numChans, outputBuffers[chn].data() + readOffset, sizeof(ProcessingType) * processOutLen, 1);
				break;
			case 2:
				CopySample<SC::ConversionChain<SC::Convert<int16, ProcessingType>, SC::DecodeIdentity<ProcessingType>>>(static_cast<int16 *>(newSampleData) + outPos + chn, processOutLen, numChans, outputBuffers[chn].data() + readOffset, sizeof(ProcessingType) * processOutLen, 1);
				break;
			}
		}

		inPos += processInLen * numChans;
		inRemain -= processInLen;
		outPos += processOutLen * numChans;
		outRemain -= processOutLen;
		prerollRemain -= readOffset;
	}

	FinishProcessing(newSampleData);

	return Result::OK;
}


std::pair<SmpLength, SmpLength> Signalsmith::CalculateBufferSizes() const
{
	// As Signalsmith Stretch implicitly determines the time-stretch factor through the ratio between input and output buffer sizes,
	// we increase the buffer sizes until the ratio between buffer sizes is close enough to the wanted ratio.
	const bool shorten = m_stretchRatio < 1.0f;
	SmpLength baseBufferSize = 8191;
	double longBufferSize;
	do
	{
		baseBufferSize++;
		longBufferSize = shorten ? baseBufferSize / m_stretchRatio : (baseBufferSize * m_stretchRatio);
	} while(longBufferSize - std::floor(longBufferSize) > 0.00001 && longBufferSize < Util::MaxValueOfType(baseBufferSize));

	if(shorten)
		return std::make_pair(static_cast<SmpLength>(longBufferSize), baseBufferSize);
	else
		return std::make_pair(baseBufferSize, static_cast<SmpLength>(longBufferSize));
}


Result LoFi::Process()
{
	if(const auto result = CheckPreconditions(); result != Result::OK)
		return result;
	if(m_grainSize < 16)
		return Result::InvalidGrainSize;
	if(m_sample.nLength < static_cast<SmpLength>(m_grainSize))
		return Result::SampleTooShort;
	
	// General idea of "Akai-style" cyclic time-stretch (not necessarily how the hardware actually does it - Akaizer produces different results):
	// We divide the sample into lots of small grains of a fixed size.
	// A grain is played and once we get close to the end of the grain, we start playing the next grain further into the sample, and cross-fade between them.
	// Pitch shift simply changes the speed at which the grains themselves are played.
	// Note that the current implementation has some problems:
	// - Flutter from crossfades may be audible
	// - Low frequencies (period > grain size) will get detuned; ringmod-like sound at very small grain sizes
	const double overlapAt = 0.8;
	const SamplePosition overlapStartSample = SamplePosition::FromDouble(m_grainSize * overlapAt);
	const SamplePosition nextGrainIncrement = SamplePosition::FromDouble(mpt::round(m_grainSize * overlapAt / (m_stretchRatio * m_pitch)));  // Always start from integer position to avoid resampling artifacts when not pitch-shifting
	const SamplePosition increment = SamplePosition::FromDouble(m_pitch);
	const SamplePosition grainSize = {m_grainSize, 0};
	const double overlapSamplesInv = 1.0 / (grainSize - overlapStartSample).ToDouble();

	struct Grain
	{
		SamplePosition offset;   // Offset into sample data
		SamplePosition playPos;  // Offset within grain

		std::pair<SmpLength, int16> GetPosition() const
		{
			const SamplePosition pos = offset + playPos;
			SmpLength intPos = pos.GetUInt();
			static_assert(SamplePosition::fractMax == uint32_max);
			const int16 fract = static_cast<int16>(pos.GetFract() >> 17);
			return std::make_pair(intPos, fract);
		}
	};
	std::array<Grain, 2> grains;
	grains[0].offset = SamplePosition{static_cast<int32>(m_start), 0};

	const auto smpSize = m_sample.GetElementarySampleSize();
	const auto numChans = m_sample.GetNumChannels();

	void *newSampleData = ModSample::AllocateSample(m_newLength, m_sample.GetBytesPerSample());
	if(newSampleData == nullptr)
		return Result::OutOfMemory;

	static const auto Interpolate = [](const auto *in, const SmpLength intPos, const int16 fract, const uint8 numChans)
	{
		int32 smp1 = in[intPos * numChans];
		int32 smp2 = in[(intPos + 1) * numChans];
		return smp1 + (smp2 - smp1) * fract / 32768;
	};

	const auto GetInterpolatedSample = [&](const std::pair<SmpLength, int16> grainPos, const uint8 chn)
	{
		const auto [intPos, fractPos] = grainPos;
		if(intPos < m_sample.nLength)
		{
			switch(smpSize)
			{
			case 1:
				return Interpolate(m_sample.sample8() + chn, intPos, fractPos, numChans);
				break;
			case 2:
				return Interpolate(m_sample.sample16() + chn, intPos, fractPos, numChans);
				break;
			}
		}
		return 0;
	};

	SmpLength outPos = m_start * numChans, outRemain = m_newSelLength;
	int grainsActive = 1;
	while(outRemain)
	{
		if(UpdateProgress(m_newSelLength - outRemain, m_newSelLength))
		{
			ModSample::FreeSample(newSampleData);
			return Result::Abort;
		}

		std::pair<SmpLength, int16> grainPos1 = grains[0].GetPosition(), grainPos2;
		double fade = 1.0;
		if(grainsActive == 2)
		{
			grainPos2 = grains[1].GetPosition();
			fade = (grains[0].playPos - overlapStartSample).ToDouble() * overlapSamplesInv;
		}

		for(uint8 chn = 0; chn < numChans; chn++)
		{
			int32 grain1 = GetInterpolatedSample(grainPos1, chn);
			if(grainsActive == 2)
			{
				int32 grain2 = GetInterpolatedSample(grainPos2, chn);
				grain1 = static_cast<int32>(mpt::round(grain1 + (grain2 - grain1) * fade));
			}

			switch(smpSize)
			{
			case 1:
				static_cast<int8 *>(newSampleData)[outPos + chn] = static_cast<int8>(grain1);
				break;
			case 2:
				static_cast<int16 *>(newSampleData)[outPos + chn] = static_cast<int16>(grain1);
				break;
			}
		}

		grains[0].playPos += increment;
		grains[1].playPos += increment;
		if(grainsActive == 1 && grains[0].playPos >= overlapStartSample)
		{
			grainsActive = 2;
			grains[1].offset = grains[0].offset + nextGrainIncrement;
			grains[1].playPos = {};
		}

		if(grains[1].playPos >= grainSize)
		{
			grainsActive = 1;
		}

		if(grainsActive == 2 && grains[0].playPos >= grainSize)
		{
			grainsActive = 1;
			grains[0] = grains[1];
		}

		outPos += numChans;
		outRemain--;
	}

	FinishProcessing(newSampleData);
	
	return Result::OK;
}


} // namespace TimeStretchPitchShift

OPENMPT_NAMESPACE_END
