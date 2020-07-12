/*
 * StreamEncoderSettings.cpp
 * -------------------------
 * Purpose: Exporting streamed music files.
 * Notes  : none
 * Authors: Joern Heusipp
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#include "stdafx.h"

#include "StreamEncoderSettings.h"

#include "TrackerSettings.h"


OPENMPT_NAMESPACE_BEGIN


static mpt::ustring GetDefaultYear()
{
	return mpt::ToUnicode(CTime::GetCurrentTime().Format(_T("%Y")));
}


StoredTags::StoredTags(SettingsContainer &conf)
	: artist(conf, U_("Export"), U_("TagArtist"), TrackerSettings::Instance().defaultArtist)
	, album(conf, U_("Export"), U_("TagAlbum"), U_(""))
	, trackno(conf, U_("Export"), U_("TagTrackNo"), U_(""))
	, year(conf, U_("Export"), U_("TagYear"), GetDefaultYear())
	, url(conf, U_("Export"), U_("TagURL"), U_(""))
	, genre(conf, U_("Export"), U_("TagGenre"), U_(""))
{
	return;
}


EncoderSettingsConf::EncoderSettingsConf(SettingsContainer &conf, const mpt::ustring &encoderName, bool cues, bool tags, uint32 samplerate, uint16 channels, Encoder::Mode mode, int bitrate, float quality, int format, int dither)
	: Cues(conf, U_("Export"), encoderName + U_("_") + U_("Cues"), cues)
	, Tags(conf, U_("Export"), encoderName + U_("_") + U_("Tags"), tags)
	, Samplerate(conf, U_("Export"), encoderName + U_("_") + U_("Samplerate"), samplerate)
	, Channels(conf, U_("Export"), encoderName + U_("_") + U_("Channels"), channels)
	, Mode(conf, U_("Export"), encoderName + U_("_") + U_("Mode"), mode)
	, Bitrate(conf, U_("Export"), encoderName + U_("_") + U_("Bitrate"), bitrate)
	, Quality(conf, U_("Export"), encoderName + U_("_") + U_("Quality"), quality)
	, Format(conf, U_("Export"), encoderName + U_("_") + U_("Format"), format)
	, Dither(conf, U_("Export"), encoderName + U_("_") + U_("Dither"), dither)
{
	return;
}


EncoderSettingsConf::operator Encoder::Settings() const
{
	Encoder::Settings result;
	result.Cues = Cues;
	result.Tags = Tags;
	result.Samplerate = Samplerate;
	result.Channels = Channels;
	result.Mode = Mode;
	result.Bitrate = Bitrate;
	result.Quality = Quality;
	result.Format = Format;
	result.Dither = Dither;
	return result;
}


StreamEncoderSettingsConf::StreamEncoderSettingsConf(SettingsContainer &conf, const mpt::ustring &section)
	: FLACCompressionLevel(conf, section, U_("FLACCompressionLevel"), Encoder::StreamSettings().FLACCompressionLevel)
	, AUPaddingAlignHint(conf, section, U_("AUPaddingAlignHint"), Encoder::StreamSettings().AUPaddingAlignHint)
	, MP3ID3v2MinPadding(conf, section, U_("MP3ID3v2MinPadding"), Encoder::StreamSettings().MP3ID3v2MinPadding)
	, MP3ID3v2PaddingAlignHint(conf, section, U_("MP3ID3v2PaddingAlignHint"), Encoder::StreamSettings().MP3ID3v2PaddingAlignHint)
	, MP3ID3v2WriteReplayGainTXXX(conf, section, U_("MP3ID3v2WriteReplayGainTXXX"), Encoder::StreamSettings().MP3ID3v2WriteReplayGainTXXX)
	, MP3LameQuality(conf, section, U_("MP3LameQuality"), Encoder::StreamSettings().MP3LameQuality)
	, MP3LameID3v2UseLame(conf, section, U_("MP3LameID3v2UseLame"), Encoder::StreamSettings().MP3LameID3v2UseLame)
	, MP3LameCalculateReplayGain(conf, section, U_("MP3LameCalculateReplayGain"), Encoder::StreamSettings().MP3LameCalculateReplayGain)
	, MP3LameCalculatePeakSample(conf, section, U_("MP3LameCalculatePeakSample"), Encoder::StreamSettings().MP3LameCalculatePeakSample)
	, OpusComplexity(conf, section, U_("OpusComplexity"), Encoder::StreamSettings().OpusComplexity)
{
	return;
}


StreamEncoderSettingsConf::operator Encoder::StreamSettings() const
{
	Encoder::StreamSettings result;
	result.FLACCompressionLevel = FLACCompressionLevel;
	result.AUPaddingAlignHint = AUPaddingAlignHint;
	result.MP3ID3v2MinPadding = MP3ID3v2MinPadding;
	result.MP3ID3v2PaddingAlignHint = MP3ID3v2PaddingAlignHint;
	result.MP3ID3v2WriteReplayGainTXXX = MP3ID3v2WriteReplayGainTXXX;
	result.MP3LameQuality = MP3LameQuality;
	result.MP3LameID3v2UseLame = MP3LameID3v2UseLame;
	result.MP3LameCalculateReplayGain = MP3LameCalculateReplayGain;
	result.MP3LameCalculatePeakSample = MP3LameCalculatePeakSample;
	result.OpusComplexity = OpusComplexity;
	return result;
}


OPENMPT_NAMESPACE_END
