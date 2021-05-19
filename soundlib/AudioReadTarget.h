/*
 * AudioReadTarget.h
 * -----------------
 * Purpose: Callback class implementations for audio data read via CSoundFile::Read.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#pragma once

#include "mptBuildSettings.h"

#include "Sndfile.h"
#include "mpt/audio/span.hpp"
#include "../soundbase/SampleFormat.h"
#include "../soundbase/SampleFormatConverters.h"
#include "../soundbase/SampleFormatCopy.h"
#include "../soundbase/Dither.h"
#include "MixerLoops.h"
#include "Mixer.h"


OPENMPT_NAMESPACE_BEGIN


template<typename Taudio_span>
class AudioTargetBuffer
	: public IAudioTarget
{
private:
	std::size_t countRendered;
	Dither &dither;
protected:
	Taudio_span outputBuffer;
public:
	AudioTargetBuffer(Taudio_span buf, Dither &dither_)
		: countRendered(0)
		, dither(dither_)
		, outputBuffer(buf)
	{
		MPT_ASSERT(SampleFormat(SampleFormatTraits<typename Taudio_span::sample_type>::sampleFormat()).IsValid());
	}
	std::size_t GetRenderedCount() const { return countRendered; }
public:
	void Process(mpt::audio_span_interleaved<MixSampleInt> buffer) override
	{
		dither.WithDither(
			[&](auto &ditherInstance)
			{
				ConvertBufferMixInternalFixedToBuffer<MixSampleIntTraits::mix_fractional_bits, false>(mpt::make_audio_span_with_offset(outputBuffer, countRendered), buffer, ditherInstance, buffer.size_channels(), buffer.size_frames());
			}
		);
		countRendered += buffer.size_frames();
	}
	void Process(mpt::audio_span_interleaved<MixSampleFloat> buffer) override
	{
		dither.WithDither(
			[&](auto &ditherInstance)
			{
				ConvertBufferMixInternalToBuffer<false>(mpt::make_audio_span_with_offset(outputBuffer, countRendered), buffer, ditherInstance, buffer.size_channels(), buffer.size_frames());
			}
		);
		countRendered += buffer.size_frames();
	}
};


template<typename Taudio_span>
class AudioTargetBufferWithGain
	: public AudioTargetBuffer<Taudio_span>
{
private:
	using Tbase = AudioTargetBuffer<Taudio_span>;
private:
	const float gainFactor;
public:
	AudioTargetBufferWithGain(Taudio_span buf, Dither &dither, float gainFactor_)
		: Tbase(buf, dither)
		, gainFactor(gainFactor_)
	{
		return;
	}
public:
	void Process(mpt::audio_span_interleaved<MixSampleInt> buffer) override
	{
		const std::size_t countRendered_ = Tbase::GetRenderedCount();
		if constexpr(!std::is_floating_point<typename Taudio_span::sample_type>::value)
		{
			int32 gainFactor16_16 = mpt::saturate_round<int32>(gainFactor * (1 << 16));
			if(gainFactor16_16 != (1<<16))
			{
				// only apply gain when != +/- 0dB
				// no clipping prevention is done here
				for(std::size_t frame = 0; frame < buffer.size_frames(); ++frame)
				{
					for(std::size_t channel = 0; channel < buffer.size_channels(); ++channel)
					{
						buffer(channel, frame) = Util::muldiv(buffer(channel, frame), gainFactor16_16, 1 << 16);
					}
				}
			}
		}
		Tbase::Process(buffer);
		if constexpr(std::is_floating_point<typename Taudio_span::sample_type>::value)
		{
			if(gainFactor != 1.0f)
			{
				// only apply gain when != +/- 0dB
				for(std::size_t frame = 0; frame < buffer.size_frames(); ++frame)
				{
					for(std::size_t channel = 0; channel < buffer.size_channels(); ++channel)
					{
						Tbase::outputBuffer(channel, countRendered_ + frame) *= gainFactor;
					}
				}
			}
		}
	}
	void Process(mpt::audio_span_interleaved<MixSampleFloat> buffer) override
	{
		if(gainFactor != 1.0f)
		{
			// only apply gain when != +/- 0dB
			for(std::size_t frame = 0; frame < buffer.size_frames(); ++frame)
			{
				for(std::size_t channel = 0; channel < buffer.size_channels(); ++channel)
				{
					buffer(channel, frame) *= gainFactor;
				}
			}
		}
		Tbase::Process(buffer);
	}
};


OPENMPT_NAMESPACE_END
