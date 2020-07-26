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

#include <ostream>


OPENMPT_NAMESPACE_BEGIN


StreamWriterBase::StreamWriterBase(std::ostream &stream)
	: f(stream)
	, fStart(f.tellp())
{
	return;
}

StreamWriterBase::~StreamWriterBase()
{
	return;
}


mpt::endian StreamWriterBase::GetConvertedEndianness() const
{
	return mpt::get_endian();
}


void StreamWriterBase::WriteInterleavedConverted(size_t frameCount, const std::byte *data)
{
	MPT_UNREFERENCED_PARAMETER(frameCount);
	MPT_UNREFERENCED_PARAMETER(data);
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
	f.write(buf.data(), buf.size());
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
