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
//------------------------------------------------------
{
	return TrackerSettings::Instance().ExportStreamEncoderSettings;
}


StreamEncoderSettings::StreamEncoderSettings(SettingsContainer &conf, const mpt::ustring &section)
//------------------------------------------------------------------------------------------------
	: FLACCompressionLevel(conf, section, "FLACCompressionLevel", 5)
	, MP3ID3v2MinPadding(conf, section, "MP3ID3v2MinPadding", 1024)
	, MP3ID3v2PaddingAlignHint(conf, section, "MP3ID3v2PaddingAlignHint", 4096)
	, MP3ID3v2WriteReplayGainTXXX(conf, section, "MP3ID3v2WriteReplayGainTXXX", true)
	, MP3LameQuality(conf, section, "MP3LameQuality", 3)
	, MP3LameID3v2UseLame(conf, section, "MP3LameID3v2UseLame", false)
	, MP3LameCalculateReplayGain(conf, section, "MP3LameCalculateReplayGain", true)
	, MP3LameCalculatePeakSample(conf, section, "MP3LameCalculatePeakSample", true)
	, MP3ACMFast(conf, section, "MP3ACMFast", false)
	, OpusComplexity(conf, section, "OpusComplexity", -1)
{
	return;
}


StreamWriterBase::StreamWriterBase(std::ostream &stream)
//------------------------------------------------------
	: f(stream)
	, fStart(f.tellp())
{
	return;
}

StreamWriterBase::~StreamWriterBase()
//-----------------------------------
{
	return;
}


void StreamWriterBase::WriteMetatags(const FileTags &tags)
//--------------------------------------------------------
{
	MPT_UNREFERENCED_PARAMETER(tags);
}


void StreamWriterBase::WriteInterleavedConverted(size_t frameCount, const char *data)
//-----------------------------------------------------------------------------------
{
	MPT_UNREFERENCED_PARAMETER(frameCount);
	MPT_UNREFERENCED_PARAMETER(data);
}


void StreamWriterBase::WriteCues(const std::vector<uint64> &cues)
//---------------------------------------------------------------
{
	MPT_UNREFERENCED_PARAMETER(cues);
}


void StreamWriterBase::WriteBuffer()
//----------------------------------
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
//---------------------------------------------------------------
{
	m_Traits = traits;
}


mpt::ustring EncoderFactoryBase::DescribeQuality(float quality) const
//-------------------------------------------------------------------
{
	return mpt::String::Print(MPT_USTRING("VBR %1%%"), static_cast<int>(quality * 100.0f));
}

mpt::ustring EncoderFactoryBase::DescribeBitrateVBR(int bitrate) const
//--------------------------------------------------------------------
{
	return mpt::String::Print(MPT_USTRING("VBR %1 kbit"), bitrate);
}

mpt::ustring EncoderFactoryBase::DescribeBitrateABR(int bitrate) const
//--------------------------------------------------------------------
{
	return mpt::String::Print(MPT_USTRING("ABR %1 kbit"), bitrate);
}

mpt::ustring EncoderFactoryBase::DescribeBitrateCBR(int bitrate) const
//--------------------------------------------------------------------
{
	return mpt::String::Print(MPT_USTRING("CBR %1 kbit"), bitrate);
}


OPENMPT_NAMESPACE_END
