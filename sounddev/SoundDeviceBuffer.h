/*
* SoundDeviceBuffer.h
* -------------------
* Purpose: Sound device buffer utilities.
* Notes  : (currently none)
* Authors: OpenMPT Devs
* The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
*/


#pragma once

#include "BuildSettings.h"

#include "SoundDevice.h"

#include "../soundbase/SampleBuffer.h"
#include "../soundbase/SampleFormatCopy.h"
#include "../soundbase/Dither.h"

#include <type_traits>


OPENMPT_NAMESPACE_BEGIN


namespace SoundDevice {



template <int fractionalBits, typename Tbuffer, typename Tsample>
inline void BufferReadTemplateFixed(Tbuffer & dst, const Tsample * src, std::size_t srcTotal, std::size_t srcPos, std::size_t numFrames, std::size_t numChannels)
{
	ConvertBufferToBufferMixFixed<fractionalBits>(dst, advance_audio_buffer(audio_buffer_interleaved<const Tsample>(src, numChannels, srcTotal), srcPos), numChannels, numFrames);
}


template <typename Tbuffer, typename Tsample>
inline void BufferReadTemplateFloat(Tbuffer & dst, const Tsample * src, std::size_t srcTotal, std::size_t srcPos, std::size_t numFrames, std::size_t numChannels)
{
	ConvertBufferToBufferMixFloat(dst, advance_audio_buffer(audio_buffer_interleaved<const Tsample>(src, numChannels, srcTotal), srcPos), numChannels, numFrames);
}


template <int fractionalBits, typename Tsample, typename Tbuffer, std::enable_if_t<!std::is_floating_point<Tsample>::value, bool> = true>
inline void BufferWriteTemplateFixed(Tsample * dst, std::size_t dstTotal, std::size_t dstPos, Tbuffer & src, std::size_t numFrames, std::size_t numChannels, Dither &dither, bool clipFloat)
{
	MPT_UNUSED_VARIABLE(clipFloat);
	dither.WithDither(
		[&](auto &ditherInstance)
		{
			ConvertBufferMixFixedToBuffer<fractionalBits, false>(advance_audio_buffer(audio_buffer_interleaved<Tsample>(dst, numChannels, dstTotal), dstPos), src, ditherInstance, numChannels, numFrames);
		}
	);
}
template <int fractionalBits, typename Tsample, typename Tbuffer, std::enable_if_t<std::is_floating_point<Tsample>::value, bool> = true>
inline void BufferWriteTemplateFixed(Tsample * dst, std::size_t dstTotal, std::size_t dstPos, Tbuffer & src, std::size_t numFrames, std::size_t numChannels, Dither &dither, bool clipFloat)
{
	if(clipFloat)
	{
		dither.WithDither(
			[&](auto &ditherInstance)
			{
				ConvertBufferMixFixedToBuffer<fractionalBits, true>(advance_audio_buffer(audio_buffer_interleaved<Tsample>(dst, numChannels, dstTotal), dstPos), src, ditherInstance, numChannels, numFrames);
			}
		);
	} else
	{
		dither.WithDither(
			[&](auto &ditherInstance)
			{
				ConvertBufferMixFixedToBuffer<fractionalBits, false>(advance_audio_buffer(audio_buffer_interleaved<Tsample>(dst, numChannels, dstTotal), dstPos), src, ditherInstance, numChannels, numFrames);
			}
		);
	}
}


template <typename Tsample, typename Tbuffer, std::enable_if_t<!std::is_floating_point<Tsample>::value, bool> = true>
void BufferWriteTemplateFloat(Tsample * dst, std::size_t dstTotal, std::size_t dstPos, Tbuffer & src, std::size_t numFrames, std::size_t numChannels, Dither &dither, bool clipFloat)
{
	MPT_UNUSED_VARIABLE(clipFloat);
	dither.WithDither(
		[&](auto &ditherInstance)
		{
			ConvertBufferMixFloatToBuffer<false>(advance_audio_buffer(audio_buffer_interleaved<Tsample>(dst, numChannels, dstTotal), dstPos), src, ditherInstance, numChannels, numFrames);
		}
	);
}
template <typename Tsample, typename Tbuffer, std::enable_if_t<std::is_floating_point<Tsample>::value, bool> = true>
void BufferWriteTemplateFloat(Tsample * dst, std::size_t dstTotal, std::size_t dstPos, Tbuffer & src, std::size_t numFrames, std::size_t numChannels, Dither &dither, bool clipFloat)
{
	if(clipFloat)
	{
		dither.WithDither(
			[&](auto &ditherInstance)
			{
				ConvertBufferMixFloatToBuffer<true>(advance_audio_buffer(audio_buffer_interleaved<Tsample>(dst, numChannels, dstTotal), dstPos), src, ditherInstance, numChannels, numFrames);
			}
		);
	} else
	{
		dither.WithDither(
			[&](auto &ditherInstance)
			{
				ConvertBufferMixFloatToBuffer<false>(advance_audio_buffer(audio_buffer_interleaved<Tsample>(dst, numChannels, dstTotal), dstPos), src, ditherInstance, numChannels, numFrames);
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
	template <typename Tbuffer>
	inline void Read(Tbuffer & dst, std::size_t countChunk)
	{
		MPT_ASSERT(m_countFramesReadProcessed + countChunk <= m_countFramesTotal);
		SoundDevice::BufferReadTemplateFloat(dst, m_src, m_countFramesTotal, m_countFramesReadProcessed, countChunk, m_bufferFormat.InputChannels);
		m_countFramesReadProcessed += countChunk;
	}
	template <int fractionalBits, typename Tbuffer>
	inline void ReadFixedPoint(Tbuffer & dst, std::size_t countChunk)
	{
		MPT_ASSERT(m_countFramesReadProcessed + countChunk <= m_countFramesTotal);
		SoundDevice::BufferReadTemplateFixed<fractionalBits>(dst, m_src, m_countFramesTotal, m_countFramesReadProcessed, countChunk, m_bufferFormat.InputChannels);
		m_countFramesReadProcessed += countChunk;
	}
	template <typename Tbuffer>
	inline void Write(Tbuffer & src, std::size_t countChunk)
	{
		MPT_ASSERT(m_countFramesWriteProcessed + countChunk <= m_countFramesTotal);
		SoundDevice::BufferWriteTemplateFloat(m_dst, m_countFramesTotal, m_countFramesWriteProcessed, src, countChunk, m_bufferFormat.Channels, m_dither, m_bufferFormat.NeedsClippedFloat);
		m_countFramesWriteProcessed += countChunk;
	}
	template <int fractionalBits, typename Tbuffer>
	inline void WriteFixedPoint(Tbuffer & src, std::size_t countChunk)
	{
		MPT_ASSERT(m_countFramesWriteProcessed + countChunk <= m_countFramesTotal);
		SoundDevice::BufferWriteTemplateFixed<fractionalBits>(m_dst, m_countFramesTotal, m_countFramesWriteProcessed, src, countChunk, m_bufferFormat.Channels, m_dither, m_bufferFormat.NeedsClippedFloat);
		m_countFramesWriteProcessed += countChunk;
	}
};


} // namespace SoundDevice


OPENMPT_NAMESPACE_END
