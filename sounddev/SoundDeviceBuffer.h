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


#include "../soundbase/SampleTypes.h"
#include "../soundbase/SampleBuffer.h"
#include "../soundbase/Dither.h"


OPENMPT_NAMESPACE_BEGIN


namespace SoundDevice {


void BufferRead(audio_buffer_interleaved<MixSampleInt> & dst, const void * src, std::size_t srcTotal, std::size_t srcPos, std::size_t numFrames, std::size_t numChannels, SampleFormat sampleFormat);
void BufferRead(audio_buffer_planar<MixSampleInt> & dst, const void * src, std::size_t srcTotal, std::size_t srcPos, std::size_t numFrames, std::size_t numChannels, SampleFormat sampleFormat);

void BufferRead(audio_buffer_interleaved<MixSampleFloat> & dst, const void * src, std::size_t srcTotal, std::size_t srcPos, std::size_t numFrames, std::size_t numChannels, SampleFormat sampleFormat);
void BufferRead(audio_buffer_planar<MixSampleFloat> & dst, const void * src, std::size_t srcTotal, std::size_t srcPos, std::size_t numFrames, std::size_t numChannels, SampleFormat sampleFormat);

void BufferWrite(void * dst, std::size_t dstTotal, std::size_t dstPos, audio_buffer_interleaved<const MixSampleInt> & src, std::size_t numFrames, std::size_t numChannels, Dither &dither, SampleFormat sampleFormat, bool clipFloat);
void BufferWrite(void * dst, std::size_t dstTotal, std::size_t dstPos, audio_buffer_planar<const MixSampleInt> & src, std::size_t numFrames, std::size_t numChannels, Dither &dither, SampleFormat sampleFormat, bool clipFloat);

void BufferWrite(void * dst, std::size_t dstTotal, std::size_t dstPos, audio_buffer_interleaved<const MixSampleFloat> & src, std::size_t numFrames, std::size_t numChannels, Dither &dither, SampleFormat sampleFormat, bool clipFloat);
void BufferWrite(void * dst, std::size_t dstTotal, std::size_t dstPos, audio_buffer_planar<const MixSampleFloat> & src, std::size_t numFrames, std::size_t numChannels, Dither &dither, SampleFormat sampleFormat, bool clipFloat);


class BufferIO
{
private:
	const void * const m_src;
	void * const m_dst;
	std::size_t m_countFramesReadProcessed;
	std::size_t m_countFramesWriteProcessed;
	Dither & m_dither;
	const BufferFormat m_bufferFormat;
	const std::size_t m_countFramesTotal;
public:
	inline BufferIO(void * dst, const void * src, std::size_t numFrames, Dither & dither, BufferFormat bufferFormat)
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
		SoundDevice::BufferRead(dst, m_src, m_countFramesTotal, m_countFramesReadProcessed, countChunk, m_bufferFormat.InputChannels, m_bufferFormat.sampleFormat);
		m_countFramesReadProcessed += countChunk;
	}
	template <typename Tbuffer>
	inline void Write(Tbuffer & src, std::size_t countChunk)
	{
		MPT_ASSERT(m_countFramesWriteProcessed + countChunk <= m_countFramesTotal);
		SoundDevice::BufferWrite(m_dst, m_countFramesTotal, m_countFramesWriteProcessed, src, countChunk, m_bufferFormat.Channels, m_dither, m_bufferFormat.sampleFormat, m_bufferFormat.NeedsClippedFloat);
		m_countFramesWriteProcessed += countChunk;
	}
};


} // namespace SoundDevice


OPENMPT_NAMESPACE_END
