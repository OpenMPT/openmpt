/*
 * AudioReadTarget.h
 * -----------------
 * Purpose: Callback class implementations for audio data read via CSoundFile::Read.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#pragma once

#include "BuildSettings.h"

#include "Sndfile.h"
#include "../soundbase/SampleFormat.h"
#include "../soundbase/SampleFormatConverters.h"
#include "../soundbase/SampleFormatCopy.h"
#include "../soundbase/SampleBuffer.h"
#include "../soundbase/Dither.h"
#include "MixerLoops.h"
#include "Mixer.h"


OPENMPT_NAMESPACE_BEGIN


template<typename Tspan>
class AudioReadTargetBuffer
	: public IAudioReadTarget
{
private:
	std::size_t countRendered;
	Dither &dither;
protected:
	Tspan outputBuffer;
public:
	AudioReadTargetBuffer(Tspan buf, Dither &dither_)
		: countRendered(0)
		, dither(dither_)
		, outputBuffer(buf)
	{
		MPT_ASSERT(SampleFormat(SampleFormatTraits<typename Tspan::sample_type>::sampleFormat()).IsValid());
	}
	std::size_t GetRenderedCount() const { return countRendered; }
public:
	void DataCallback(MixSampleInt *MixSoundBuffer, std::size_t channels, std::size_t countChunk) override
	{
		dither.WithDither(
			[&](auto &ditherInstance)
			{
				ConvertBufferMixFixedToBuffer<MixSampleIntTraits::mix_fractional_bits, false>(make_audio_span_with_offset(outputBuffer, countRendered), audio_span_interleaved<MixSampleInt>(MixSoundBuffer, channels, countChunk), ditherInstance, channels, countChunk);
			}
		);
		countRendered += countChunk;
	}
	void DataCallback(MixSampleFloat *MixSoundBuffer, std::size_t channels, std::size_t countChunk) override
	{
		dither.WithDither(
			[&](auto &ditherInstance)
			{
				ConvertBufferMixFloatToBuffer<false>(make_audio_span_with_offset(outputBuffer, countRendered), audio_span_interleaved<MixSampleFloat>(MixSoundBuffer, channels, countChunk), ditherInstance, channels, countChunk);
			}
		);
		countRendered += countChunk;
	}
};


template<typename Tspan>
class AudioReadTargetGainBuffer
	: public AudioReadTargetBuffer<Tspan>
{
private:
	typedef AudioReadTargetBuffer<Tspan> Tbase;
private:
	const float gainFactor;
public:
	AudioReadTargetGainBuffer(Tspan buf, Dither &dither, float gainFactor_)
		: Tbase(buf, dither)
		, gainFactor(gainFactor_)
	{
		return;
	}
public:
	void DataCallback(MixSampleInt *MixSoundBuffer, std::size_t channels, std::size_t countChunk) override
	{
		const std::size_t countRendered_ = Tbase::GetRenderedCount();
		if constexpr(!std::is_floating_point<typename Tspan::sample_type>::value)
		{
			int32 gainFactor16_16 = mpt::saturate_round<int32>(gainFactor * (1 << 16));
			if(gainFactor16_16 != (1<<16))
			{
				// only apply gain when != +/- 0dB
				// no clipping prevention is done here
				MixSampleInt *buf = MixSoundBuffer;
				for(std::size_t i = 0; i < countChunk * channels; ++i)
				{
					*buf = Util::muldiv(*buf, gainFactor16_16, 1<<16);
					buf++;
				}
			}
		}
		Tbase::DataCallback(MixSoundBuffer, channels, countChunk);
		if constexpr(std::is_floating_point<typename Tspan::sample_type>::value)
		{
			if(gainFactor != 1.0f)
			{
				// only apply gain when != +/- 0dB
				for(std::size_t i = 0; i < countChunk; ++i)
				{
					for(std::size_t channel = 0; channel < channels; ++channel)
					{
						Tbase::outputBuffer(channel, countRendered_ + i) *= gainFactor;
					}
				}
			}
		}
	}
	void DataCallback(MixSampleFloat *MixSoundBuffer, std::size_t channels, std::size_t countChunk) override
	{
		if(gainFactor != 1.0f)
		{
			// only apply gain when != +/- 0dB
			MixSampleFloat *buf = MixSoundBuffer;
			for(std::size_t i = 0; i < countChunk * channels; ++i)
			{
				*buf *= gainFactor;
				buf++;
			}
		}
		Tbase::DataCallback(MixSoundBuffer, channels, countChunk);
	}
};


OPENMPT_NAMESPACE_END
