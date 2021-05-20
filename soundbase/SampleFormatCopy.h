/*
 * SampleFormatCopy.h
 * ------------------
 * Purpose: Functions for copying sample data.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "mptBuildSettings.h"


#include "../common/Endianness.h"
#include "SampleFormatConverters.h"
#include "SampleFormat.h"
#include "mpt/audio/span.hpp"


OPENMPT_NAMESPACE_BEGIN


//////////////////////////////////////////////////////
// Actual sample conversion functions


template <typename TBufOut, typename TBufIn>
void CopyAudio(TBufOut buf_out, TBufIn buf_in)
{
	MPT_ASSERT(buf_in.size_frames() == buf_out.size_frames());
	MPT_ASSERT(buf_in.size_channels() == buf_out.size_channels());
	std::size_t countFrames = std::min(buf_in.size_frames(), buf_out.size_frames());
	std::size_t channels = std::min(buf_in.size_channels(), buf_out.size_channels());
	for(std::size_t frame = 0; frame < countFrames; ++frame)
	{
		for(std::size_t channel = 0; channel < channels; ++channel)
		{
			buf_out(channel, frame) = SC::sample_cast<typename TBufOut::sample_type>(buf_in(channel, frame));
		}
	}
}


template <typename TBufOut, typename TBufIn>
void CopyAudio(TBufOut buf_out, TBufIn buf_in, std::size_t countFrames)
{
	MPT_ASSERT(countFrames <= buf_in.size_frames());
	MPT_ASSERT(countFrames <= buf_out.size_frames());
	MPT_ASSERT(buf_in.size_channels() == buf_out.size_channels());
	std::size_t channels = std::min(buf_in.size_channels(), buf_out.size_channels());
	for(std::size_t frame = 0; frame < countFrames; ++frame)
	{
		for(std::size_t channel = 0; channel < channels; ++channel)
		{
			buf_out(channel, frame) = SC::sample_cast<typename TBufOut::sample_type>(buf_in(channel, frame));
		}
	}
}


template <typename TBufOut, typename TBufIn>
void CopyAudioChannels(TBufOut buf_out, TBufIn buf_in, std::size_t channels, std::size_t countFrames)
{
	MPT_ASSERT(countFrames <= buf_in.size_frames());
	MPT_ASSERT(countFrames <= buf_out.size_frames());
	MPT_ASSERT(channels <= buf_in.size_channels());
	MPT_ASSERT(channels <= buf_out.size_channels());
	for(std::size_t frame = 0; frame < countFrames; ++frame)
	{
		for(std::size_t channel = 0; channel < channels; ++channel)
		{
			buf_out(channel, frame) = SC::sample_cast<typename TBufOut::sample_type>(buf_in(channel, frame));
		}
	}
}


// Copy numChannels interleaved sample streams.
template <typename Tin, typename Tout>
void CopyAudioChannelsInterleaved(Tout *MPT_RESTRICT outBuf, const Tin *MPT_RESTRICT inBuf, std::size_t numChannels, std::size_t countFrames)
{
	CopyAudio(mpt::audio_span_interleaved<Tout>(outBuf, numChannels, countFrames), mpt::audio_span_interleaved<const Tin>(inBuf, numChannels, countFrames));
}


// Copy a sample data buffer.
// targetBuffer: Buffer in which the sample should be copied into.
// numSamples: Number of samples of size T that should be copied. targetBuffer is expected to be able to hold "numSamples * incTarget" samples.
// incTarget: Number of samples by which the target data pointer is increased each time.
// sourceBuffer: Buffer from which the samples should be read.
// sourceSize: Size of source buffer, in bytes.
// incSource: Number of samples by which the source data pointer is increased each time.
//
// Template arguments:
// SampleConversion: Functor of type SampleConversionFunctor to apply sample conversion (see above for existing functors).
template <typename SampleConversion>
size_t CopySample(typename SampleConversion::output_t *MPT_RESTRICT outBuf, size_t numSamples, size_t incTarget, const typename SampleConversion::input_t *MPT_RESTRICT inBuf, size_t sourceSize, size_t incSource, SampleConversion conv = SampleConversion())
{
	const size_t sampleSize = incSource * SampleConversion::input_inc * sizeof(typename SampleConversion::input_t);
	LimitMax(numSamples, sourceSize / sampleSize);
	const size_t copySize = numSamples * sampleSize;

	SampleConversion sampleConv(conv);
	while(numSamples--)
	{
		*outBuf = sampleConv(inBuf);
		outBuf += incTarget;
		inBuf += incSource * SampleConversion::input_inc;
	}

	return copySize;
}


template <int fractionalBits, bool clipOutput, typename TOutBuf, typename TInBuf, typename Tdither>
void ConvertBufferMixInternalFixedToBuffer(TOutBuf outBuf, TInBuf inBuf, Tdither &dither, std::size_t channels, std::size_t count)
{
	using TOutSample = typename std::remove_const<typename TOutBuf::sample_type>::type;
	using TInSample = typename std::remove_const<typename TInBuf::sample_type>::type;
	MPT_ASSERT(inBuf.size_channels() >= channels);
	MPT_ASSERT(outBuf.size_channels() >= channels);
	MPT_ASSERT(inBuf.size_frames() >= count);
	MPT_ASSERT(outBuf.size_frames() >= count);
	constexpr int ditherBits = SampleFormat(SampleFormatTraits<TOutSample>::sampleFormat()).IsInt()
		? SampleFormat(SampleFormatTraits<TOutSample>::sampleFormat()).GetBitsPerSample()
		: 0;
	SC::ClipFixed<int32, fractionalBits, clipOutput> clip;
	SC::ConvertFixedPoint<TOutSample, TInSample, fractionalBits> conv;
	for(std::size_t i = 0; i < count; ++i)
	{
		for(std::size_t channel = 0; channel < channels; ++channel)
		{
			outBuf(channel, i) = conv(clip(dither.template process<ditherBits>(channel, inBuf(channel, i))));
		}
	}
}


template <int fractionalBits, typename TOutBuf, typename TInBuf>
void ConvertBufferToBufferMixInternalFixed(TOutBuf outBuf, TInBuf inBuf, std::size_t channels, std::size_t count)
{
	using TOutSample = typename std::remove_const<typename TOutBuf::sample_type>::type;
	using TInSample = typename std::remove_const<typename TInBuf::sample_type>::type;
	MPT_ASSERT(inBuf.size_channels() >= channels);
	MPT_ASSERT(outBuf.size_channels() >= channels);
	MPT_ASSERT(inBuf.size_frames() >= count);
	MPT_ASSERT(outBuf.size_frames() >= count);
	SC::ConvertToFixedPoint<TOutSample, TInSample, fractionalBits> conv;
	for(std::size_t i = 0; i < count; ++i)
	{
		for(std::size_t channel = 0; channel < channels; ++channel)
		{
			outBuf(channel, i) = conv(inBuf(channel, i));
		}
	}
}


template <bool clipOutput, typename TOutBuf, typename TInBuf, typename Tdither>
void ConvertBufferMixInternalToBuffer(TOutBuf outBuf, TInBuf inBuf, Tdither &dither, std::size_t channels, std::size_t count)
{
	using TOutSample = typename std::remove_const<typename TOutBuf::sample_type>::type;
	using TInSample = typename std::remove_const<typename TInBuf::sample_type>::type;
	MPT_ASSERT(inBuf.size_channels() >= channels);
	MPT_ASSERT(outBuf.size_channels() >= channels);
	MPT_ASSERT(inBuf.size_frames() >= count);
	MPT_ASSERT(outBuf.size_frames() >= count);
	constexpr int ditherBits = SampleFormat(SampleFormatTraits<TOutSample>::sampleFormat()).IsInt()
		? SampleFormat(SampleFormatTraits<TOutSample>::sampleFormat()).GetBitsPerSample()
		: 0;
	SC::Clip<TInSample, clipOutput> clip;
	SC::Convert<TOutSample, TInSample> conv;
	for(std::size_t i = 0; i < count; ++i)
	{
		for(std::size_t channel = 0; channel < channels; ++channel)
		{
			outBuf(channel, i) = conv(clip(dither.template process<ditherBits>(channel, inBuf(channel, i))));
		}
	}
}


template <typename TOutBuf, typename TInBuf>
void ConvertBufferToBufferMixInternal(TOutBuf outBuf, TInBuf inBuf, std::size_t channels, std::size_t count)
{
	using TOutSample = typename std::remove_const<typename TOutBuf::sample_type>::type;
	using TInSample = typename std::remove_const<typename TInBuf::sample_type>::type;
	MPT_ASSERT(inBuf.size_channels() >= channels);
	MPT_ASSERT(outBuf.size_channels() >= channels);
	MPT_ASSERT(inBuf.size_frames() >= count);
	MPT_ASSERT(outBuf.size_frames() >= count);
	SC::Convert<TOutSample, TInSample> conv;
	for(std::size_t i = 0; i < count; ++i)
	{
		for(std::size_t channel = 0; channel < channels; ++channel)
		{
			outBuf(channel, i) = conv(inBuf(channel, i));
		}
	}
}


OPENMPT_NAMESPACE_END
