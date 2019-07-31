/*
 * SoundDeviceBuffer.cpp
 * ---------------------
 * Purpose: Sound device buffer utilities.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"

#include "SoundDevice.h"

#include "../soundbase/SampleTypes.h"
#include "../soundbase/SampleBuffer.h"
#include "../soundbase/SampleFormatCopy.h"
#include "../soundbase/Dither.h"


OPENMPT_NAMESPACE_BEGIN


namespace SoundDevice {


template <typename Tbuffer>
static void BufferReadTemplateFixed(Tbuffer & dst, const void * src, std::size_t srcTotal, std::size_t srcPos, std::size_t numFrames, std::size_t numChannels, SampleFormat sampleFormat)
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
static void BufferReadTemplateFloat(Tbuffer & dst, const void * src, std::size_t srcTotal, std::size_t srcPos, std::size_t numFrames, std::size_t numChannels, SampleFormat sampleFormat)
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
static void BufferWriteTemplateFixed(void * dst, std::size_t dstTotal, std::size_t dstPos, Tbuffer & src, std::size_t numFrames, std::size_t numChannels, Dither &dither, SampleFormat sampleFormat, bool clipFloat)
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
static void BufferWriteTemplateFloat(void * dst, std::size_t dstTotal, std::size_t dstPos, Tbuffer & src, std::size_t numFrames, std::size_t numChannels, Dither &dither, SampleFormat sampleFormat, bool clipFloat)
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



void BufferRead(audio_buffer_interleaved<MixSampleInt> & dst, const void * src, std::size_t srcTotal, std::size_t srcPos, std::size_t numFrames, std::size_t numChannels, SampleFormat sampleFormat)
{
	BufferReadTemplateFixed(dst, src, srcTotal, srcPos, numFrames, numChannels, sampleFormat);
}

void BufferRead(audio_buffer_planar<MixSampleInt> & dst, const void * src, std::size_t srcTotal, std::size_t srcPos, std::size_t numFrames, std::size_t numChannels, SampleFormat sampleFormat)
{
	BufferReadTemplateFixed(dst, src, srcTotal, srcPos, numFrames, numChannels, sampleFormat);
}

void BufferRead(audio_buffer_interleaved<MixSampleFloat> & dst, const void * src, std::size_t srcTotal, std::size_t srcPos, std::size_t numFrames, std::size_t numChannels, SampleFormat sampleFormat)
{
	BufferReadTemplateFloat(dst, src, srcTotal, srcPos, numFrames, numChannels, sampleFormat);
}

void BufferRead(audio_buffer_planar<MixSampleFloat> & dst, const void * src, std::size_t srcTotal, std::size_t srcPos, std::size_t numFrames, std::size_t numChannels, SampleFormat sampleFormat)
{
	BufferReadTemplateFloat(dst, src, srcTotal, srcPos, numFrames, numChannels, sampleFormat);
}

void BufferWrite(void * dst, std::size_t dstTotal, std::size_t dstPos, audio_buffer_interleaved<const MixSampleInt> & src, std::size_t numFrames, std::size_t numChannels, Dither &dither, SampleFormat sampleFormat, bool clipFloat)
{
	BufferWriteTemplateFixed(dst, dstTotal, dstPos, src, numFrames, numChannels, dither, sampleFormat, clipFloat);
}

void BufferWrite(void * dst, std::size_t dstTotal, std::size_t dstPos, audio_buffer_planar<const MixSampleInt> & src, std::size_t numFrames, std::size_t numChannels, Dither &dither, SampleFormat sampleFormat, bool clipFloat)
{
	BufferWriteTemplateFixed(dst, dstTotal, dstPos, src, numFrames, numChannels, dither, sampleFormat, clipFloat);
}

void BufferWrite(void * dst, std::size_t dstTotal, std::size_t dstPos, audio_buffer_interleaved<const MixSampleFloat> & src, std::size_t numFrames, std::size_t numChannels, Dither &dither, SampleFormat sampleFormat, bool clipFloat)
{
	BufferWriteTemplateFloat(dst, dstTotal, dstPos, src, numFrames, numChannels, dither, sampleFormat, clipFloat);
}

void BufferWrite(void * dst, std::size_t dstTotal, std::size_t dstPos, audio_buffer_planar<const MixSampleFloat> & src, std::size_t numFrames, std::size_t numChannels, Dither &dither, SampleFormat sampleFormat, bool clipFloat)
{
	BufferWriteTemplateFloat(dst, dstTotal, dstPos, src, numFrames, numChannels, dither, sampleFormat, clipFloat);
}


} // namespace SoundDevice


OPENMPT_NAMESPACE_END
