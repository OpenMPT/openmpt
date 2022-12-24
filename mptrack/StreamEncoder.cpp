/*
 * StreamEncoder.cpp
 * -----------------
 * Purpose: Exporting streamed music files.
 * Notes  : none
 * Authors: Joern Heusipp
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#include "stdafx.h"

#include "StreamEncoder.h"

#include "mpt/endian/floatingpoint.hpp"
#include "mpt/endian/int24.hpp"
#include "mpt/endian/integer.hpp"
#include "mpt/endian/type_traits.hpp"
#include "mpt/io/base.hpp"
#include "mpt/io/io.hpp"
#include "mpt/io/io_stdstream.hpp"
#include "mpt/io_write/buffer.hpp"

#include "openmpt/soundbase/SampleEncode.hpp"
#include "openmpt/soundbase/SampleFormat.hpp"

#include <ostream>


OPENMPT_NAMESPACE_BEGIN


StreamWriterBase::StreamWriterBase(std::ostream &stream)
	: f(stream)
	, fStart(mpt::IO::TellWrite(f))
{
	return;
}

StreamWriterBase::~StreamWriterBase()
{
	return;
}



template <mpt::endian endian, typename Tsample>
static inline std::pair<bool, std::size_t> WriteInterleavedImpl(std::ostream &f, uint16 channels, Encoder::Format format, std::size_t frameCount, const Tsample *interleaved)
{
	MPT_ASSERT(endian == format.endian);
	MPT_ASSERT(format.GetSampleFormat() == SampleFormatTraits<Tsample>::sampleFormat());
	bool success = true;
	std::size_t written = 0;
	MPT_MAYBE_CONSTANT_IF(endian == mpt::get_endian() && format.encoding != Encoder::Format::Encoding::Alaw && format.encoding != Encoder::Format::Encoding::ulaw)
	{
		if(!mpt::IO::WriteRaw(f, reinterpret_cast<const std::byte*>(interleaved), frameCount * channels * format.GetSampleFormat().GetSampleSize()))
		{
			success = false;
		}
		written += frameCount * channels * format.GetSampleFormat().GetSampleSize();
	} else
	{
		std::array<std::byte, mpt::IO::BUFFERSIZE_TINY> fbuf;
		mpt::IO::WriteBuffer<std::ostream> bf{f, fbuf};
		if(format.encoding == Encoder::Format::Encoding::Alaw)
		{
			if constexpr(std::is_same<Tsample, int16>::value)
			{
				SC::EncodeALaw conv;
				for(std::size_t frame = 0; frame < frameCount; ++frame)
				{
					for(uint16 channel = 0; channel < channels; ++channel)
					{
						std::byte sampledata = conv(interleaved[channel]);
						mpt::IO::WriteRaw(bf, &sampledata, 1);
						written += 1;
					}
					interleaved += channels;
				}
			}
		} else if(format.encoding == Encoder::Format::Encoding::ulaw)
		{
			if constexpr(std::is_same<Tsample, int16>::value)
			{
				SC::EncodeuLaw conv;
				for(std::size_t frame = 0; frame < frameCount; ++frame)
				{
					for(uint16 channel = 0; channel < channels; ++channel)
					{
						std::byte sampledata = conv(interleaved[channel]);
						mpt::IO::WriteRaw(bf, &sampledata, 1);
						written += 1;
					}
					interleaved += channels;
				}
			}
		} else
		{
			for(std::size_t frame = 0; frame < frameCount; ++frame)
			{
				for(uint16 channel = 0; channel < channels; ++channel)
				{
					typename mpt::make_endian<endian, Tsample>::type sample{};
					sample = interleaved[channel];
					mpt::IO::Write(bf, sample);
				}
				written += channels * format.GetSampleFormat().GetSampleSize();
				interleaved += channels;
			}
		}
		if(bf.HasWriteError())
		{
			success = false;
		}
	}
	return std::make_pair(success, written);
}

std::pair<bool, std::size_t> WriteInterleavedLE(std::ostream &f, uint16 channels, Encoder::Format format, std::size_t frameCount, const double *interleaved)
{
	MPT_ASSERT(format.endian == mpt::endian::little);
	return WriteInterleavedImpl<mpt::endian::little>(f, channels, format, frameCount, interleaved);
}
std::pair<bool, std::size_t> WriteInterleavedLE(std::ostream &f, uint16 channels, Encoder::Format format, std::size_t frameCount, const float *interleaved)
{
	MPT_ASSERT(format.endian == mpt::endian::little);
	return WriteInterleavedImpl<mpt::endian::little>(f, channels, format, frameCount, interleaved);
}
std::pair<bool, std::size_t> WriteInterleavedLE(std::ostream &f, uint16 channels, Encoder::Format format, std::size_t frameCount, const int32 *interleaved)
{
	MPT_ASSERT(format.endian == mpt::endian::little);
	return WriteInterleavedImpl<mpt::endian::little>(f, channels, format, frameCount, interleaved);
}
std::pair<bool, std::size_t> WriteInterleavedLE(std::ostream &f, uint16 channels, Encoder::Format format, std::size_t frameCount, const int24 *interleaved)
{
	MPT_ASSERT(format.endian == mpt::endian::little);
	return WriteInterleavedImpl<mpt::endian::little>(f, channels, format, frameCount, interleaved);
}
std::pair<bool, std::size_t> WriteInterleavedLE(std::ostream &f, uint16 channels, Encoder::Format format, std::size_t frameCount, const int16 *interleaved)
{
	MPT_ASSERT(format.endian == mpt::endian::little);
	return WriteInterleavedImpl<mpt::endian::little>(f, channels, format, frameCount, interleaved);
}
std::pair<bool, std::size_t> WriteInterleavedLE(std::ostream &f, uint16 channels, Encoder::Format format, std::size_t frameCount, const int8 *interleaved)
{
	MPT_ASSERT(format.endian == mpt::endian::little);
	return WriteInterleavedImpl<mpt::endian::little>(f, channels, format, frameCount, interleaved);
}
std::pair<bool, std::size_t> WriteInterleavedLE(std::ostream &f, uint16 channels, Encoder::Format format, std::size_t frameCount, const uint8 *interleaved)
{
	MPT_ASSERT(format.endian == mpt::endian::little);
	return WriteInterleavedImpl<mpt::endian::little>(f, channels, format, frameCount, interleaved);
}

std::pair<bool, std::size_t> WriteInterleavedBE(std::ostream &f, uint16 channels, Encoder::Format format, std::size_t frameCount, const double *interleaved)
{
	MPT_ASSERT(format.endian == mpt::endian::big);
	return WriteInterleavedImpl<mpt::endian::big>(f, channels, format, frameCount, interleaved);
}
std::pair<bool, std::size_t> WriteInterleavedBE(std::ostream &f, uint16 channels, Encoder::Format format, std::size_t frameCount, const float *interleaved)
{
	MPT_ASSERT(format.endian == mpt::endian::big);
	return WriteInterleavedImpl<mpt::endian::big>(f, channels, format, frameCount, interleaved);
}
std::pair<bool, std::size_t> WriteInterleavedBE(std::ostream &f, uint16 channels, Encoder::Format format, std::size_t frameCount, const int32 *interleaved)
{
	MPT_ASSERT(format.endian == mpt::endian::big);
	return WriteInterleavedImpl<mpt::endian::big>(f, channels, format, frameCount, interleaved);
}
std::pair<bool, std::size_t> WriteInterleavedBE(std::ostream &f, uint16 channels, Encoder::Format format, std::size_t frameCount, const int24 *interleaved)
{
	MPT_ASSERT(format.endian == mpt::endian::big);
	return WriteInterleavedImpl<mpt::endian::big>(f, channels, format, frameCount, interleaved);
}
std::pair<bool, std::size_t> WriteInterleavedBE(std::ostream &f, uint16 channels, Encoder::Format format, std::size_t frameCount, const int16 *interleaved)
{
	MPT_ASSERT(format.endian == mpt::endian::big);
	return WriteInterleavedImpl<mpt::endian::big>(f, channels, format, frameCount, interleaved);
}
std::pair<bool, std::size_t> WriteInterleavedBE(std::ostream &f, uint16 channels, Encoder::Format format, std::size_t frameCount, const int8 *interleaved)
{
	MPT_ASSERT(format.endian == mpt::endian::big);
	return WriteInterleavedImpl<mpt::endian::big>(f, channels, format, frameCount, interleaved);
}
std::pair<bool, std::size_t> WriteInterleavedBE(std::ostream &f, uint16 channels, Encoder::Format format, std::size_t frameCount, const uint8 *interleaved)
{
	MPT_ASSERT(format.endian == mpt::endian::big);
	return WriteInterleavedImpl<mpt::endian::big>(f, channels, format, frameCount, interleaved);
}



SampleFormat StreamWriterBase::GetSampleFormat() const
{
	return SampleFormat::Float32;
}


void StreamWriterBase::WriteInterleaved(std::size_t frameCount, const double *interleaved)
{
	MPT_UNREFERENCED_PARAMETER(frameCount);
	MPT_UNREFERENCED_PARAMETER(interleaved);
	MPT_ASSERT_NOTREACHED();
}

void StreamWriterBase::WriteInterleaved(std::size_t frameCount, const float *interleaved)
{
	MPT_UNREFERENCED_PARAMETER(frameCount);
	MPT_UNREFERENCED_PARAMETER(interleaved);
	MPT_ASSERT_NOTREACHED();
}

void StreamWriterBase::WriteInterleaved(std::size_t frameCount, const int32 *interleaved)
{
	MPT_UNREFERENCED_PARAMETER(frameCount);
	MPT_UNREFERENCED_PARAMETER(interleaved);
	MPT_ASSERT_NOTREACHED();
}

void StreamWriterBase::WriteInterleaved(std::size_t frameCount, const int24 *interleaved)
{
	MPT_UNREFERENCED_PARAMETER(frameCount);
	MPT_UNREFERENCED_PARAMETER(interleaved);
	MPT_ASSERT_NOTREACHED();
}

void StreamWriterBase::WriteInterleaved(std::size_t frameCount, const int16 *interleaved)
{
	MPT_UNREFERENCED_PARAMETER(frameCount);
	MPT_UNREFERENCED_PARAMETER(interleaved);
	MPT_ASSERT_NOTREACHED();
}

void StreamWriterBase::WriteInterleaved(std::size_t frameCount, const int8 *interleaved)
{
	MPT_UNREFERENCED_PARAMETER(frameCount);
	MPT_UNREFERENCED_PARAMETER(interleaved);
	MPT_ASSERT_NOTREACHED();
}

void StreamWriterBase::WriteInterleaved(std::size_t frameCount, const uint8 *interleaved)
{
	MPT_UNREFERENCED_PARAMETER(frameCount);
	MPT_UNREFERENCED_PARAMETER(interleaved);
	MPT_ASSERT_NOTREACHED();
}


void StreamWriterBase::WriteCues(const std::vector<uint64> &cues)
{
	MPT_UNREFERENCED_PARAMETER(cues);
}


void StreamWriterBase::WriteFinalize()
{
	return;
}


void StreamWriterBase::WriteBuffer()
{
	if(!f)
	{
		return;
	}
	if(buf.empty())
	{
		return;
	}
	mpt::IO::WriteRaw(f, mpt::as_span(buf));
	buf.resize(0);
}


void EncoderFactoryBase::SetTraits(const Encoder::Traits &traits)
{
	m_Traits = traits;
}


bool EncoderFactoryBase::IsBitrateSupported(int samplerate, int channels, int bitrate) const
{
	MPT_UNREFERENCED_PARAMETER(samplerate);
	MPT_UNREFERENCED_PARAMETER(channels);
	MPT_UNREFERENCED_PARAMETER(bitrate);
	return true;
}


mpt::ustring EncoderFactoryBase::DescribeQuality(float quality) const
{
	return MPT_UFORMAT("VBR {}%")(static_cast<int>(quality * 100.0f));
}

mpt::ustring EncoderFactoryBase::DescribeBitrateVBR(int bitrate) const
{
	return MPT_UFORMAT("VBR {} kbit")(bitrate);
}

mpt::ustring EncoderFactoryBase::DescribeBitrateABR(int bitrate) const
{
	return MPT_UFORMAT("ABR {} kbit")(bitrate);
}

mpt::ustring EncoderFactoryBase::DescribeBitrateCBR(int bitrate) const
{
	return MPT_UFORMAT("CBR {} kbit")(bitrate);
}


OPENMPT_NAMESPACE_END
