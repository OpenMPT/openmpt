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
#include "../soundbase/SampleFormatConverters.h"
#include "../soundbase/SampleFormatCopy.h"
#include "../soundbase/Dither.h"

#include <type_traits>


OPENMPT_NAMESPACE_BEGIN


namespace SoundDevice {


template <typename Tsample, typename TDither>
class BufferIO
{

private:
	mpt::audio_span_interleaved<const Tsample> const m_src;
	mpt::audio_span_interleaved<Tsample> const m_dst;
	std::size_t m_countFramesReadProcessed;
	std::size_t m_countFramesWriteProcessed;
	TDither & m_dither;
	const BufferFormat m_bufferFormat;

public:
	inline BufferIO(Tsample * dst, const Tsample * src, std::size_t numFrames, TDither & dither, BufferFormat bufferFormat)
		: m_src(src, bufferFormat.InputChannels, numFrames)
		, m_dst(dst, bufferFormat.Channels, numFrames)
		, m_countFramesReadProcessed(0)
		, m_countFramesWriteProcessed(0)
		, m_dither(dither)
		, m_bufferFormat(bufferFormat)
	{
		return;
	}

	template <typename audio_span_dst>
	inline void Read(audio_span_dst dst)
	{
		MPT_ASSERT(m_countFramesReadProcessed + dst.size_frames() <= m_src.size_frames());
		ConvertBufferToBufferMixInternal(dst, mpt::make_audio_span_with_offset(m_src, m_countFramesReadProcessed), m_bufferFormat.InputChannels, dst.size_frames());
		m_countFramesReadProcessed += dst.size_frames();
	}

	template <int fractionalBits, typename audio_span_dst>
	inline void ReadFixedPoint(audio_span_dst dst)
	{
		MPT_ASSERT(m_countFramesReadProcessed + dst.size_frames() <= m_src.size_frames());
		ConvertBufferToBufferMixInternalFixed<fractionalBits>(dst, mpt::make_audio_span_with_offset(m_src, m_countFramesReadProcessed), m_bufferFormat.InputChannels, dst.size_frames());
		m_countFramesReadProcessed += dst.size_frames();
	}

	template <typename audio_span_src>
	inline void Write(audio_span_src src)
	{
		MPT_ASSERT(m_countFramesWriteProcessed + src.size_frames() <= m_dst.size_frames());
		if(m_bufferFormat.WantsClippedOutput)
		{
			ConvertBufferMixInternalToBuffer<true>(mpt::make_audio_span_with_offset(m_dst, m_countFramesWriteProcessed), src, m_dither, m_bufferFormat.Channels, src.size_frames());
		} else
		{
			ConvertBufferMixInternalToBuffer<false>(mpt::make_audio_span_with_offset(m_dst, m_countFramesWriteProcessed), src, m_dither, m_bufferFormat.Channels, src.size_frames());
		}
		m_countFramesWriteProcessed += src.size_frames();
	}

	template <int fractionalBits, typename audio_span_src>
	inline void WriteFixedPoint(audio_span_src src)
	{
		MPT_ASSERT(m_countFramesWriteProcessed + src.size_frames() <= m_dst.size_frames());
		if(m_bufferFormat.WantsClippedOutput)
		{
			ConvertBufferMixInternalFixedToBuffer<fractionalBits, true>(mpt::make_audio_span_with_offset(m_dst, m_countFramesWriteProcessed), src, m_dither, m_bufferFormat.Channels, src.size_frames());
		} else
		{
			ConvertBufferMixInternalFixedToBuffer<fractionalBits, false>(mpt::make_audio_span_with_offset(m_dst, m_countFramesWriteProcessed), src, m_dither, m_bufferFormat.Channels, src.size_frames());
		}
		m_countFramesWriteProcessed += src.size_frames();
	}

	inline ~BufferIO()
	{
		// fill remaining buffer with silence
		while(m_countFramesWriteProcessed < m_dst.size_frames())
		{
			for(std::size_t channel = 0; channel < m_dst.size_channels(); ++channel)
			{
				m_dst(channel, m_countFramesWriteProcessed) = SC::sample_cast<Tsample>(static_cast<int16>(0));
			}
			m_countFramesWriteProcessed += 1;
		}
	}

};


} // namespace SoundDevice


OPENMPT_NAMESPACE_END
