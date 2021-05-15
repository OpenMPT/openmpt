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

#include "../soundbase/SampleBuffer.h"
#include "../soundbase/SampleFormatCopy.h"
#include "../soundbase/Dither.h"

#include <type_traits>


OPENMPT_NAMESPACE_BEGIN


namespace SoundDevice {



template <int fractionalBits, typename Tspan, typename Tsample>
inline void BufferReadTemplateFixed(Tspan & dst, const Tsample * src, std::size_t srcTotal, std::size_t srcPos, std::size_t numFrames, std::size_t numChannels)
{
	ConvertBufferToBufferMixFixed<fractionalBits>(dst, make_audio_span_with_offset(audio_span_interleaved<const Tsample>(src, numChannels, srcTotal), srcPos), numChannels, numFrames);
}


template <typename Tspan, typename Tsample>
inline void BufferReadTemplateFloat(Tspan & dst, const Tsample * src, std::size_t srcTotal, std::size_t srcPos, std::size_t numFrames, std::size_t numChannels)
{
	ConvertBufferToBufferMixFloat(dst, make_audio_span_with_offset(audio_span_interleaved<const Tsample>(src, numChannels, srcTotal), srcPos), numChannels, numFrames);
}


template <int fractionalBits, typename Tsample, typename Tspan, std::enable_if_t<!std::is_floating_point<Tsample>::value, bool> = true>
inline void BufferWriteTemplateFixed(Tsample * dst, std::size_t dstTotal, std::size_t dstPos, Tspan & src, std::size_t numFrames, std::size_t numChannels, Dither &dither, bool clipFloat)
{
	MPT_UNUSED_VARIABLE(clipFloat);
	dither.WithDither(
		[&](auto &ditherInstance)
		{
			ConvertBufferMixFixedToBuffer<fractionalBits, false>(make_audio_span_with_offset(audio_span_interleaved<Tsample>(dst, numChannels, dstTotal), dstPos), src, ditherInstance, numChannels, numFrames);
		}
	);
}
template <int fractionalBits, typename Tsample, typename Tspan, std::enable_if_t<std::is_floating_point<Tsample>::value, bool> = true>
inline void BufferWriteTemplateFixed(Tsample * dst, std::size_t dstTotal, std::size_t dstPos, Tspan & src, std::size_t numFrames, std::size_t numChannels, Dither &dither, bool clipFloat)
{
	if(clipFloat)
	{
		dither.WithDither(
			[&](auto &ditherInstance)
			{
				ConvertBufferMixFixedToBuffer<fractionalBits, true>(make_audio_span_with_offset(audio_span_interleaved<Tsample>(dst, numChannels, dstTotal), dstPos), src, ditherInstance, numChannels, numFrames);
			}
		);
	} else
	{
		dither.WithDither(
			[&](auto &ditherInstance)
			{
				ConvertBufferMixFixedToBuffer<fractionalBits, false>(make_audio_span_with_offset(audio_span_interleaved<Tsample>(dst, numChannels, dstTotal), dstPos), src, ditherInstance, numChannels, numFrames);
			}
		);
	}
}


template <typename Tsample, typename Tspan, std::enable_if_t<!std::is_floating_point<Tsample>::value, bool> = true>
void BufferWriteTemplateFloat(Tsample * dst, std::size_t dstTotal, std::size_t dstPos, Tspan & src, std::size_t numFrames, std::size_t numChannels, Dither &dither, bool clipFloat)
{
	MPT_UNUSED_VARIABLE(clipFloat);
	dither.WithDither(
		[&](auto &ditherInstance)
		{
			ConvertBufferMixFloatToBuffer<false>(make_audio_span_with_offset(audio_span_interleaved<Tsample>(dst, numChannels, dstTotal), dstPos), src, ditherInstance, numChannels, numFrames);
		}
	);
}
template <typename Tsample, typename Tspan, std::enable_if_t<std::is_floating_point<Tsample>::value, bool> = true>
void BufferWriteTemplateFloat(Tsample * dst, std::size_t dstTotal, std::size_t dstPos, Tspan & src, std::size_t numFrames, std::size_t numChannels, Dither &dither, bool clipFloat)
{
	if(clipFloat)
	{
		dither.WithDither(
			[&](auto &ditherInstance)
			{
				ConvertBufferMixFloatToBuffer<true>(make_audio_span_with_offset(audio_span_interleaved<Tsample>(dst, numChannels, dstTotal), dstPos), src, ditherInstance, numChannels, numFrames);
			}
		);
	} else
	{
		dither.WithDither(
			[&](auto &ditherInstance)
			{
				ConvertBufferMixFloatToBuffer<false>(make_audio_span_with_offset(audio_span_interleaved<Tsample>(dst, numChannels, dstTotal), dstPos), src, ditherInstance, numChannels, numFrames);
			}
		);
	}
}



template <typename Tsample>
class BufferIO
{
private:
	const Tsample * const m_src;
	Tsample * const m_dst;
	std::size_t m_countFramesReadProcessed;
	std::size_t m_countFramesWriteProcessed;
	Dither & m_dither;
	const BufferFormat m_bufferFormat;
	const std::size_t m_countFramesTotal;
public:
	inline BufferIO(Tsample * dst, const Tsample * src, std::size_t numFrames, Dither & dither, BufferFormat bufferFormat)
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
		SoundDevice::BufferReadTemplateFloat(dst, m_src, m_countFramesTotal, m_countFramesReadProcessed, countChunk, m_bufferFormat.InputChannels);
		m_countFramesReadProcessed += countChunk;
	}
	template <int fractionalBits, typename Tspan>
	inline void ReadFixedPoint(Tspan & dst, std::size_t countChunk)
	{
		MPT_ASSERT(m_countFramesReadProcessed + countChunk <= m_countFramesTotal);
		SoundDevice::BufferReadTemplateFixed<fractionalBits>(dst, m_src, m_countFramesTotal, m_countFramesReadProcessed, countChunk, m_bufferFormat.InputChannels);
		m_countFramesReadProcessed += countChunk;
	}
	template <typename Tspan>
	inline void Write(Tspan & src, std::size_t countChunk)
	{
		MPT_ASSERT(m_countFramesWriteProcessed + countChunk <= m_countFramesTotal);
		SoundDevice::BufferWriteTemplateFloat(m_dst, m_countFramesTotal, m_countFramesWriteProcessed, src, countChunk, m_bufferFormat.Channels, m_dither, m_bufferFormat.NeedsClippedFloat);
		m_countFramesWriteProcessed += countChunk;
	}
	template <int fractionalBits, typename Tspan>
	inline void WriteFixedPoint(Tspan & src, std::size_t countChunk)
	{
		MPT_ASSERT(m_countFramesWriteProcessed + countChunk <= m_countFramesTotal);
		SoundDevice::BufferWriteTemplateFixed<fractionalBits>(m_dst, m_countFramesTotal, m_countFramesWriteProcessed, src, countChunk, m_bufferFormat.Channels, m_dither, m_bufferFormat.NeedsClippedFloat);
		m_countFramesWriteProcessed += countChunk;
	}
};


} // namespace SoundDevice


OPENMPT_NAMESPACE_END
