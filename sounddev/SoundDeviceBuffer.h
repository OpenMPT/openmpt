/*
* SoundDeviceBuffer.h
* -------------------
* Purpose: Sound device buffer utilities.
* Notes  : (currently none)
* Authors: OpenMPT Devs
* The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
*/


#pragma once

#include "mptBuildSettings.h"

#include "SoundDevice.h"

#include "mpt/audio/span.hpp"
#include "../soundbase/SampleFormatCopy.h"
#include "../soundbase/Dither.h"

#include <type_traits>


OPENMPT_NAMESPACE_BEGIN


namespace SoundDevice {



template <int fractionalBits, typename Tspan, typename Tsample>
inline void BufferReadFixed(Tspan & dst, const Tsample * src, std::size_t srcTotal, std::size_t srcPos, std::size_t numFrames, std::size_t numChannels)
{
	ConvertBufferToBufferMixInternalFixed<fractionalBits>(dst, mpt::make_audio_span_with_offset(mpt::audio_span_interleaved<const Tsample>(src, numChannels, srcTotal), srcPos), numChannels, numFrames);
}


template <typename Tspan, typename Tsample>
inline void BufferRead(Tspan & dst, const Tsample * src, std::size_t srcTotal, std::size_t srcPos, std::size_t numFrames, std::size_t numChannels)
{
	ConvertBufferToBufferMixInternal(dst, mpt::make_audio_span_with_offset(mpt::audio_span_interleaved<const Tsample>(src, numChannels, srcTotal), srcPos), numChannels, numFrames);
}


template <int fractionalBits, typename Tsample, typename Tspan, typename TDither>
inline void BufferWriteFixed(Tsample * dst, std::size_t dstTotal, std::size_t dstPos, Tspan & src, std::size_t numFrames, std::size_t numChannels, TDither &dither, bool clipOutput)
{
	if(clipOutput)
	{
		ConvertBufferMixInternalFixedToBuffer<fractionalBits, true>(mpt::make_audio_span_with_offset(mpt::audio_span_interleaved<Tsample>(dst, numChannels, dstTotal), dstPos), src, dither, numChannels, numFrames);
	} else
	{
		ConvertBufferMixInternalFixedToBuffer<fractionalBits, false>(mpt::make_audio_span_with_offset(mpt::audio_span_interleaved<Tsample>(dst, numChannels, dstTotal), dstPos), src, dither, numChannels, numFrames);
	}
}


template <typename Tsample, typename Tspan, typename TDither>
void BufferWrite(Tsample * dst, std::size_t dstTotal, std::size_t dstPos, Tspan & src, std::size_t numFrames, std::size_t numChannels, TDither &dither, bool clipOutput)
{
	if(clipOutput)
	{
		ConvertBufferMixInternalToBuffer<true>(mpt::make_audio_span_with_offset(mpt::audio_span_interleaved<Tsample>(dst, numChannels, dstTotal), dstPos), src, dither, numChannels, numFrames);
	} else
	{
		ConvertBufferMixInternalToBuffer<false>(mpt::make_audio_span_with_offset(mpt::audio_span_interleaved<Tsample>(dst, numChannels, dstTotal), dstPos), src, dither, numChannels, numFrames);
	}
}



template <typename Tsample, typename TDither>
class BufferIO
{
private:
	const Tsample * const m_src;
	Tsample * const m_dst;
	std::size_t m_countFramesReadProcessed;
	std::size_t m_countFramesWriteProcessed;
	TDither & m_dither;
	const BufferFormat m_bufferFormat;
	const std::size_t m_countFramesTotal;
public:
	inline BufferIO(Tsample * dst, const Tsample * src, std::size_t numFrames, TDither & dither, BufferFormat bufferFormat)
		: m_src(src)
		, m_dst(dst)
		, m_countFramesReadProcessed(0)
		, m_countFramesWriteProcessed(0)
		, m_dither(dither)
		, m_bufferFormat(bufferFormat)
		, m_countFramesTotal(numFrames)
	{
		return;
	}
	template <typename Tspan>
	inline void Read(Tspan & dst, std::size_t countChunk)
	{
		MPT_ASSERT(m_countFramesReadProcessed + countChunk <= m_countFramesTotal);
		SoundDevice::BufferRead(dst, m_src, m_countFramesTotal, m_countFramesReadProcessed, countChunk, m_bufferFormat.InputChannels);
		m_countFramesReadProcessed += countChunk;
	}
	template <int fractionalBits, typename Tspan>
	inline void ReadFixedPoint(Tspan & dst, std::size_t countChunk)
	{
		MPT_ASSERT(m_countFramesReadProcessed + countChunk <= m_countFramesTotal);
		SoundDevice::BufferReadFixed<fractionalBits>(dst, m_src, m_countFramesTotal, m_countFramesReadProcessed, countChunk, m_bufferFormat.InputChannels);
		m_countFramesReadProcessed += countChunk;
	}
	template <typename Tspan>
	inline void Write(Tspan & src, std::size_t countChunk)
	{
		MPT_ASSERT(m_countFramesWriteProcessed + countChunk <= m_countFramesTotal);
		SoundDevice::BufferWrite(m_dst, m_countFramesTotal, m_countFramesWriteProcessed, src, countChunk, m_bufferFormat.Channels, m_dither, m_bufferFormat.NeedsClippedFloat);
		m_countFramesWriteProcessed += countChunk;
	}
	template <int fractionalBits, typename Tspan>
	inline void WriteFixedPoint(Tspan & src, std::size_t countChunk)
	{
		MPT_ASSERT(m_countFramesWriteProcessed + countChunk <= m_countFramesTotal);
		SoundDevice::BufferWriteFixed<fractionalBits>(m_dst, m_countFramesTotal, m_countFramesWriteProcessed, src, countChunk, m_bufferFormat.Channels, m_dither, m_bufferFormat.NeedsClippedFloat);
		m_countFramesWriteProcessed += countChunk;
	}
};


} // namespace SoundDevice


OPENMPT_NAMESPACE_END
