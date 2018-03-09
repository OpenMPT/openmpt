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

#include "Mptrack.h"
#include "TrackerSettings.h"

#include <ostream>


OPENMPT_NAMESPACE_BEGIN


StreamEncoderSettings &StreamEncoderSettings::Instance()
{
	return TrackerSettings::Instance().ExportStreamEncoderSettings;
}


StreamEncoderSettings::StreamEncoderSettings(SettingsContainer &conf, const mpt::ustring &section)
	: FLACCompressionLevel(conf, section, MPT_USTRING("FLACCompressionLevel"), 5)
	, AUPaddingAlignHint(conf, section, MPT_USTRING("AUPaddingAlignHint"), 4096)
	, MP3ID3v2MinPadding(conf, section, MPT_USTRING("MP3ID3v2MinPadding"), 1024)
	, MP3ID3v2PaddingAlignHint(conf, section, MPT_USTRING("MP3ID3v2PaddingAlignHint"), 4096)
	, MP3ID3v2WriteReplayGainTXXX(conf, section, MPT_USTRING("MP3ID3v2WriteReplayGainTXXX"), true)
	, MP3LameQuality(conf, section, MPT_USTRING("MP3LameQuality"), 3)
	, MP3LameID3v2UseLame(conf, section, MPT_USTRING("MP3LameID3v2UseLame"), false)
	, MP3LameCalculateReplayGain(conf, section, MPT_USTRING("MP3LameCalculateReplayGain"), true)
	, MP3LameCalculatePeakSample(conf, section, MPT_USTRING("MP3LameCalculatePeakSample"), true)
	, MP3ACMFast(conf, section, MPT_USTRING("MP3ACMFast"), false)
	, OpusComplexity(conf, section, MPT_USTRING("OpusComplexity"), -1)
{
	return;
}


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


mpt::endian_type StreamWriterBase::GetConvertedEndianness() const
{
	return mpt::endian();
}


void StreamWriterBase::WriteInterleavedConverted(size_t frameCount, const char *data)
{
	MPT_UNREFERENCED_PARAMETER(frameCount);
	MPT_UNREFERENCED_PARAMETER(data);
}


void StreamWriterBase::WriteCues(const std::vector<uint64> &cues)
{
	MPT_UNREFERENCED_PARAMETER(cues);
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
	return mpt::format(MPT_USTRING("VBR %1%%"))(static_cast<int>(quality * 100.0f));
}

mpt::ustring EncoderFactoryBase::DescribeBitrateVBR(int bitrate) const
{
	return mpt::format(MPT_USTRING("VBR %1 kbit"))(bitrate);
}

mpt::ustring EncoderFactoryBase::DescribeBitrateABR(int bitrate) const
{
	return mpt::format(MPT_USTRING("ABR %1 kbit"))(bitrate);
}

mpt::ustring EncoderFactoryBase::DescribeBitrateCBR(int bitrate) const
{
	return mpt::format(MPT_USTRING("CBR %1 kbit"))(bitrate);
}


OPENMPT_NAMESPACE_END
