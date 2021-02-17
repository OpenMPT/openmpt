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
#include "../soundbase/SampleFormatCopy.h"
#include "../soundbase/Dither.h"


OPENMPT_NAMESPACE_BEGIN


namespace SoundDevice {



template <typename Tbuffer>
void BufferReadTemplateFixed(Tbuffer & dst, const void * src, std::size_t srcTotal, std::size_t srcPos, std::size_t numFrames, std::size_t numChannels, SampleFormat sampleFormat)
{
	switch(sampleFormat)
	{
	case SampleFormatUnsigned8:
		ConvertBufferToBufferMixFixed<MixSampleIntTraits::mix_fractional_bits()>(dst, advance_audio_buffer(audio_buffer_interleaved<const uint8>(static_cast<const uint8*>(src), numChannels, srcTotal), srcPos), numChannels, numFrames);
		break;
	case SampleFormatInt8:
		ConvertBufferToBufferMixFixed<MixSampleIntTraits::mix_fractional_bits()>(dst, advance_audio_buffer(audio_buffer_interleaved<const int8>(static_cast<const int8*>(src), numChannels, srcTotal), srcPos), numChannels, numFrames);
		break;
	case SampleFormatInt16:
		ConvertBufferToBufferMixFixed<MixSampleIntTraits::mix_fractional_bits()>(dst, advance_audio_buffer(audio_buffer_interleaved<const int16>(static_cast<const int16*>(src), numChannels, srcTotal), srcPos), numChannels, numFrames);
		break;
	case SampleFormatInt24:
		ConvertBufferToBufferMixFixed<MixSampleIntTraits::mix_fractional_bits()>(dst, advance_audio_buffer(audio_buffer_interleaved<const int24>(static_cast<const int24*>(src), numChannels, srcTotal), srcPos), numChannels, numFrames);
		break;
	case SampleFormatInt32:
		ConvertBufferToBufferMixFixed<MixSampleIntTraits::mix_fractional_bits()>(dst, advance_audio_buffer(audio_buffer_interleaved<const int32>(static_cast<const int32*>(src), numChannels, srcTotal), srcPos), numChannels, numFrames);
		break;
	case SampleFormatFloat32:
		ConvertBufferToBufferMixFixed<MixSampleIntTraits::mix_fractional_bits()>(dst, advance_audio_buffer(audio_buffer_interleaved<const float>(static_cast<const float*>(src), numChannels, srcTotal), srcPos), numChannels, numFrames);
		break;
	case SampleFormatFloat64:
		ConvertBufferToBufferMixFixed<MixSampleIntTraits::mix_fractional_bits()>(dst, advance_audio_buffer(audio_buffer_interleaved<const double>(static_cast<const double*>(src), numChannels, srcTotal), srcPos), numChannels, numFrames);
		break;
	case SampleFormatInvalid:
		// nothing
		break;
	}
}


template <typename Tbuffer>
void BufferReadTemplateFloat(Tbuffer & dst, const void * src, std::size_t srcTotal, std::size_t srcPos, std::size_t numFrames, std::size_t numChannels, SampleFormat sampleFormat)
{
	switch(sampleFormat)
	{
	case SampleFormatUnsigned8:
		ConvertBufferToBufferMixFloat(dst, advance_audio_buffer(audio_buffer_interleaved<const uint8>(static_cast<const uint8*>(src), numChannels, srcTotal), srcPos), numChannels, numFrames);
		break;
	case SampleFormatInt8:
		ConvertBufferToBufferMixFloat(dst, advance_audio_buffer(audio_buffer_interleaved<const int8>(static_cast<const int8*>(src), numChannels, srcTotal), srcPos), numChannels, numFrames);
		break;
	case SampleFormatInt16:
		ConvertBufferToBufferMixFloat(dst, advance_audio_buffer(audio_buffer_interleaved<const int16>(static_cast<const int16*>(src), numChannels, srcTotal), srcPos), numChannels, numFrames);
		break;
	case SampleFormatInt24:
		ConvertBufferToBufferMixFloat(dst, advance_audio_buffer(audio_buffer_interleaved<const int24>(static_cast<const int24*>(src), numChannels, srcTotal), srcPos), numChannels, numFrames);
		break;
	case SampleFormatInt32:
		ConvertBufferToBufferMixFloat(dst, advance_audio_buffer(audio_buffer_interleaved<const int32>(static_cast<const int32*>(src), numChannels, srcTotal), srcPos), numChannels, numFrames);
		break;
	case SampleFormatFloat32:
		ConvertBufferToBufferMixFloat(dst, advance_audio_buffer(audio_buffer_interleaved<const float>(static_cast<const float*>(src), numChannels, srcTotal), srcPos), numChannels, numFrames);
		break;
	case SampleFormatFloat64:
		ConvertBufferToBufferMixFloat(dst, advance_audio_buffer(audio_buffer_interleaved<const double>(static_cast<const double*>(src), numChannels, srcTotal), srcPos), numChannels, numFrames);
		break;
	case SampleFormatInvalid:
		// nothing
		break;
	}
}


template <typename Tbuffer>
void BufferWriteTemplateFixed(void * dst, std::size_t dstTotal, std::size_t dstPos, Tbuffer & src, std::size_t numFrames, std::size_t numChannels, Dither &dither, SampleFormat sampleFormat, bool clipFloat)
{
	switch(sampleFormat)
	{
		case SampleFormatUnsigned8:
			dither.WithDither(
				[&](auto &ditherInstance)
				{
					ConvertBufferMixFixedToBuffer<MixSampleIntTraits::mix_fractional_bits(), false>(advance_audio_buffer(audio_buffer_interleaved<uint8>(static_cast<uint8*>(dst), numChannels, dstTotal), dstPos), src, ditherInstance, numChannels, numFrames);
				}
			);
			break;
		case SampleFormatInt8:
			dither.WithDither(
				[&](auto &ditherInstance)
				{
					ConvertBufferMixFixedToBuffer<MixSampleIntTraits::mix_fractional_bits(), false>(advance_audio_buffer(audio_buffer_interleaved<int8>(static_cast<int8*>(dst), numChannels, dstTotal), dstPos), src, ditherInstance, numChannels, numFrames);
				}
			);
			break;
		case SampleFormatInt16:
			dither.WithDither(
				[&](auto &ditherInstance)
				{
					ConvertBufferMixFixedToBuffer<MixSampleIntTraits::mix_fractional_bits(), false>(advance_audio_buffer(audio_buffer_interleaved<int16>(static_cast<int16*>(dst), numChannels, dstTotal), dstPos), src, ditherInstance, numChannels, numFrames);
				}
			);
			break;
		case SampleFormatInt24:
			dither.WithDither(
				[&](auto &ditherInstance)
				{
					ConvertBufferMixFixedToBuffer<MixSampleIntTraits::mix_fractional_bits(), false>(advance_audio_buffer(audio_buffer_interleaved<int24>(static_cast<int24*>(dst), numChannels, dstTotal), dstPos), src, ditherInstance, numChannels, numFrames);
				}
			);
			break;
		case SampleFormatInt32:
			dither.WithDither(
				[&](auto &ditherInstance)
				{
					ConvertBufferMixFixedToBuffer<MixSampleIntTraits::mix_fractional_bits(), false>(advance_audio_buffer(audio_buffer_interleaved<int32>(static_cast<int32*>(dst), numChannels, dstTotal), dstPos), src, ditherInstance, numChannels, numFrames);
				}
			);
			break;
		case SampleFormatFloat32:
			if(clipFloat)
			{
				dither.WithDither(
					[&](auto &ditherInstance)
					{
						ConvertBufferMixFixedToBuffer<MixSampleIntTraits::mix_fractional_bits(), true>(advance_audio_buffer(audio_buffer_interleaved<float>(static_cast<float*>(dst), numChannels, dstTotal), dstPos), src, ditherInstance, numChannels, numFrames);
					}
				);
			} else
			{
				dither.WithDither(
					[&](auto &ditherInstance)
					{
						ConvertBufferMixFixedToBuffer<MixSampleIntTraits::mix_fractional_bits(), false>(advance_audio_buffer(audio_buffer_interleaved<float>(static_cast<float*>(dst), numChannels, dstTotal), dstPos), src, ditherInstance, numChannels, numFrames);
					}
				);
			}
			break;
		case SampleFormatFloat64:
			if(clipFloat)
			{
				dither.WithDither(
					[&](auto &ditherInstance)
					{
						ConvertBufferMixFixedToBuffer<MixSampleIntTraits::mix_fractional_bits(), true>(advance_audio_buffer(audio_buffer_interleaved<double>(static_cast<double*>(dst), numChannels, dstTotal), dstPos), src, ditherInstance, numChannels, numFrames);
					}
				);
			} else
			{
				dither.WithDither(
					[&](auto &ditherInstance)
					{
						ConvertBufferMixFixedToBuffer<MixSampleIntTraits::mix_fractional_bits(), false>(advance_audio_buffer(audio_buffer_interleaved<double>(static_cast<double*>(dst), numChannels, dstTotal), dstPos), src, ditherInstance, numChannels, numFrames);
					}
				);
			}
			break;
		case SampleFormatInvalid:
			// nothing
			break;
	}
}


template <typename Tbuffer>
void BufferWriteTemplateFloat(void * dst, std::size_t dstTotal, std::size_t dstPos, Tbuffer & src, std::size_t numFrames, std::size_t numChannels, Dither &dither, SampleFormat sampleFormat, bool clipFloat)
{
	switch(sampleFormat)
	{
	case SampleFormatUnsigned8:
		dither.WithDither(
			[&](auto &ditherInstance)
			{
				ConvertBufferMixFloatToBuffer<false>(advance_audio_buffer(audio_buffer_interleaved<uint8>(static_cast<uint8*>(dst), numChannels, dstTotal), dstPos), src, ditherInstance, numChannels, numFrames);
			}
		);
		break;
	case SampleFormatInt8:
		dither.WithDither(
			[&](auto &ditherInstance)
			{
				ConvertBufferMixFloatToBuffer<false>(advance_audio_buffer(audio_buffer_interleaved<int8>(static_cast<int8*>(dst), numChannels, dstTotal), dstPos), src, ditherInstance, numChannels, numFrames);
			}
		);
		break;
	case SampleFormatInt16:
		dither.WithDither(
			[&](auto &ditherInstance)
			{
				ConvertBufferMixFloatToBuffer<false>(advance_audio_buffer(audio_buffer_interleaved<int16>(static_cast<int16*>(dst), numChannels, dstTotal), dstPos), src, ditherInstance, numChannels, numFrames);
			}
		);
		break;
	case SampleFormatInt24:
		dither.WithDither(
			[&](auto &ditherInstance)
			{
				ConvertBufferMixFloatToBuffer<false>(advance_audio_buffer(audio_buffer_interleaved<int24>(static_cast<int24*>(dst), numChannels, dstTotal), dstPos), src, ditherInstance, numChannels, numFrames);
			}
		);
		break;
	case SampleFormatInt32:
		dither.WithDither(
			[&](auto &ditherInstance)
			{
				ConvertBufferMixFloatToBuffer<false>(advance_audio_buffer(audio_buffer_interleaved<int32>(static_cast<int32*>(dst), numChannels, dstTotal), dstPos), src, ditherInstance, numChannels, numFrames);
			}
		);
		break;
	case SampleFormatFloat32:
		if(clipFloat)
		{
			dither.WithDither(
				[&](auto &ditherInstance)
				{
					ConvertBufferMixFloatToBuffer<true>(advance_audio_buffer(audio_buffer_interleaved<float>(static_cast<float*>(dst), numChannels, dstTotal), dstPos), src, ditherInstance, numChannels, numFrames);
				}
			);
		} else
		{
			dither.WithDither(
				[&](auto &ditherInstance)
				{
					ConvertBufferMixFloatToBuffer<false>(advance_audio_buffer(audio_buffer_interleaved<float>(static_cast<float*>(dst), numChannels, dstTotal), dstPos), src, ditherInstance, numChannels, numFrames);
				}
			);
		}
		break;
	case SampleFormatFloat64:
		if(clipFloat)
		{
			dither.WithDither(
				[&](auto &ditherInstance)
				{
					ConvertBufferMixFloatToBuffer<true>(advance_audio_buffer(audio_buffer_interleaved<double>(static_cast<double*>(dst), numChannels, dstTotal), dstPos), src, ditherInstance, numChannels, numFrames);
				}
			);
		} else
		{
			dither.WithDither(
				[&](auto &ditherInstance)
				{
					ConvertBufferMixFloatToBuffer<false>(advance_audio_buffer(audio_buffer_interleaved<double>(static_cast<double*>(dst), numChannels, dstTotal), dstPos), src, ditherInstance, numChannels, numFrames);
				}
			);
		}
		break;
	case SampleFormatInvalid:
		// nothing
		break;
	}
}



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
		SoundDevice::BufferReadTemplateFloat(dst, m_src, m_countFramesTotal, m_countFramesReadProcessed, countChunk, m_bufferFormat.InputChannels, m_bufferFormat.sampleFormat);
		m_countFramesReadProcessed += countChunk;
	}
	template <typename Tbuffer>
	inline void ReadFixedPoint(Tbuffer & dst, std::size_t countChunk)
	{
		MPT_ASSERT(m_countFramesReadProcessed + countChunk <= m_countFramesTotal);
		SoundDevice::BufferReadTemplateFixed(dst, m_src, m_countFramesTotal, m_countFramesReadProcessed, countChunk, m_bufferFormat.InputChannels, m_bufferFormat.sampleFormat);
		m_countFramesReadProcessed += countChunk;
	}
	template <typename Tbuffer>
	inline void Write(Tbuffer & src, std::size_t countChunk)
	{
		MPT_ASSERT(m_countFramesWriteProcessed + countChunk <= m_countFramesTotal);
		SoundDevice::BufferWriteTemplateFloat(m_dst, m_countFramesTotal, m_countFramesWriteProcessed, src, countChunk, m_bufferFormat.Channels, m_dither, m_bufferFormat.sampleFormat, m_bufferFormat.NeedsClippedFloat);
		m_countFramesWriteProcessed += countChunk;
	}
	template <typename Tbuffer>
	inline void WriteFixedPoint(Tbuffer & src, std::size_t countChunk)
	{
		MPT_ASSERT(m_countFramesWriteProcessed + countChunk <= m_countFramesTotal);
		SoundDevice::BufferWriteTemplateFixed(m_dst, m_countFramesTotal, m_countFramesWriteProcessed, src, countChunk, m_bufferFormat.Channels, m_dither, m_bufferFormat.sampleFormat, m_bufferFormat.NeedsClippedFloat);
		m_countFramesWriteProcessed += countChunk;
	}
};


} // namespace SoundDevice


OPENMPT_NAMESPACE_END
