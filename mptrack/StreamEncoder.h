/*
 * StreamEncoder.h
 * ---------------
 * Purpose: Exporting streamed music files.
 * Notes  : none
 * Authors: Joern Heusipp
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#pragma once

#include "Settings.h"

#include "../soundlib/Tagging.h"
#include "../soundlib/SampleFormat.h"

#include <iosfwd>
#include <string>
#include <vector>


OPENMPT_NAMESPACE_BEGIN


static const int opus_bitrates [] = {
	8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 192, 224, 256, 320, 384, 448,      510
};
static const int vorbis_bitrates [] = {
	           32,     48,     64, 80, 96, 112, 128,      160, 192, 224, 256, 320,           500
};
static const int layer3_bitrates [] = {
	8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 192, 224, 256, 320
};
static const int mpeg1layer3_bitrates [] = {
	           32, 40, 48, 56, 64, 80, 96, 112, 128,      160, 192, 224, 256, 320
};
static const uint32 opus_samplerates [] = {
	48000,
	24000,           16000,
	12000,            8000
};
static const uint32 vorbis_samplerates [] = {
	 48000,  44100,  32000,
	 24000,  22050,  16000,
	 12000,  11025,   8000
};
static const uint32 layer3_samplerates [] = {
	 48000,  44100,  32000,
	 24000,  22050,  16000
};
static const uint32 mpeg1layer3_samplerates [] = {
	 48000,  44100,  32000
};


namespace Encoder
{

	struct Format
	{
		uint32 Samplerate;
		int Channels;
		SampleFormat Sampleformat;

		int Bitrate;
		mpt::ustring Description;
	};

	enum Mode
	{
		ModeCBR        = 1<<0,
		ModeABR        = 1<<1,
		ModeVBR        = 1<<2,
		ModeQuality    = 1<<3,
		ModeEnumerated = 1<<4,
		ModeInvalid    = 0
	};

} // namespace Encoder

template<> inline SettingValue ToSettingValue(const Encoder::Mode &val)
{
	switch(val)
	{
		case Encoder::ModeCBR: return SettingValue(MPT_USTRING("CBR"), "Encoder::Mode"); break;
		case Encoder::ModeABR: return SettingValue(MPT_USTRING("ABR"), "Encoder::Mode"); break;
		case Encoder::ModeVBR: return SettingValue(MPT_USTRING("VBR"), "Encoder::Mode"); break;
		case Encoder::ModeQuality: return SettingValue(MPT_USTRING("Quality"), "Encoder::Mode"); break;
		case Encoder::ModeEnumerated: return SettingValue(MPT_USTRING("Enumerated"), "Encoder::Mode"); break;
		default: return SettingValue(MPT_USTRING("Invalid"), "Encoder::Mode"); break;
	}
}
template<> inline Encoder::Mode FromSettingValue(const SettingValue &val)
{
	ASSERT(val.GetTypeTag() == "Encoder::Mode");
	if(val.as<mpt::ustring>() == MPT_USTRING("")) { return Encoder::ModeInvalid; }
	else if(val.as<mpt::ustring>() == MPT_USTRING("CBR")) { return Encoder::ModeCBR; }
	else if(val.as<mpt::ustring>() == MPT_USTRING("ABR")) { return Encoder::ModeABR; }
	else if(val.as<mpt::ustring>() == MPT_USTRING("VBR")) { return Encoder::ModeVBR; }
	else if(val.as<mpt::ustring>() == MPT_USTRING("Quality")) { return Encoder::ModeQuality; }
	else if(val.as<mpt::ustring>() == MPT_USTRING("Enumerated")) { return Encoder::ModeEnumerated; }
	else { return Encoder::ModeInvalid; }
}

namespace Encoder
{

	struct Traits
	{
		
		mpt::PathString fileExtension;
		mpt::ustring fileDescription;
		mpt::ustring fileShortDescription;
		mpt::ustring encoderSettingsName;
		mpt::ustring encoderName;
		mpt::ustring description;

		bool canTags;
		std::vector<mpt::ustring> genres;
		int modesWithFixedGenres;
		
		bool canCues;

		int maxChannels;
		std::vector<uint32> samplerates;
		
		int modes;
		std::vector<int> bitrates;
		std::vector<Encoder::Format> formats;
		
		uint32 defaultSamplerate;
		uint16 defaultChannels;

		Encoder::Mode defaultMode;
		int defaultBitrate;
		float defaultQuality;
		int defaultFormat;
		int defaultDitherType;

		Traits()
			: canCues(false)
			, canTags(false)
			, modesWithFixedGenres(0)
			, maxChannels(0)
			, modes(Encoder::ModeInvalid)
			, defaultSamplerate(44100)
			, defaultChannels(2)
			, defaultMode(Encoder::ModeInvalid)
			, defaultBitrate(0)
			, defaultQuality(0.0f)
			, defaultFormat(0)
			, defaultDitherType(1)
		{
			return;
		}

	};

	struct Settings
	{
		
		Setting<bool> Cues;
		Setting<bool> Tags;

		Setting<uint32> Samplerate;
		Setting<uint16> Channels;

		Setting<Encoder::Mode> Mode;
		Setting<int> Bitrate;
		Setting<float> Quality;
		Setting<int> Format;
		Setting<int> Dither;
		
		Settings(SettingsContainer &conf, const mpt::ustring &encoderName, bool cues, bool tags, uint32 samplerate, uint16 channels, Encoder::Mode mode, int bitrate, float quality, int format, int dither)
			: Cues(conf, "Export", encoderName + MPT_USTRING("_") + MPT_USTRING("Cues"), cues)
			, Tags(conf, "Export", encoderName + MPT_USTRING("_") + MPT_USTRING("Tags"), tags)
			, Samplerate(conf, "Export", encoderName + MPT_USTRING("_") + MPT_USTRING("Samplerate"), samplerate)
			, Channels(conf, "Export", encoderName + MPT_USTRING("_") + MPT_USTRING("Channels"), channels)
			, Mode(conf, "Export", encoderName + MPT_USTRING("_") + MPT_USTRING("Mode"), mode)
			, Bitrate(conf, "Export", encoderName + MPT_USTRING("_") + MPT_USTRING("Bitrate"), bitrate)
			, Quality(conf, "Export", encoderName + MPT_USTRING("_") + MPT_USTRING("Quality"), quality)
			, Format(conf, "Export", encoderName + MPT_USTRING("_") + MPT_USTRING("Format"), format)
			, Dither(conf, "Export", encoderName + MPT_USTRING("_") + MPT_USTRING("Dither"), dither)
		{
			return;
		}

	};

} // namespace Encoder


//==========================
struct StreamEncoderSettings
//==========================
{
	Setting<int32> FLACCompressionLevel;
	Setting<uint32> MP3ID3v2MinPadding;
	Setting<uint32> MP3ID3v2PaddingAlignHint;
	Setting<bool> MP3ID3v2WriteReplayGainTXXX;
	Setting<int32> MP3LameQuality;
	Setting<bool> MP3LameID3v2UseLame;
	Setting<bool> MP3LameCalculateReplayGain;
	Setting<bool> MP3LameCalculatePeakSample;
	Setting<bool> MP3ACMFast;
	Setting<int32> OpusComplexity;
	StreamEncoderSettings(SettingsContainer &conf, const mpt::ustring &section);
	static StreamEncoderSettings &Instance();
};


//=======================
class IAudioStreamEncoder
//=======================
{
protected:
	IAudioStreamEncoder() { }
public:
	virtual ~IAudioStreamEncoder() { }
public:
	// Call the following functions exactly in this order.
	virtual void SetFormat(const Encoder::Settings &settings) = 0;
	virtual void WriteMetatags(const FileTags &tags) = 0; // optional
	virtual void WriteInterleaved(size_t count, const float *interleaved) = 0;
	virtual void WriteInterleavedConverted(size_t frameCount, const char *data) = 0;
	virtual void WriteCues(const std::vector<uint64> &cues) = 0; // optional
	virtual void Finalize() = 0;
};


//====================
class StreamWriterBase
//====================
	: public IAudioStreamEncoder
{
protected:	
	std::ostream &f;
	std::streampos fStart;
	std::vector<char> buf;
public:
	StreamWriterBase(std::ostream &stream);
	virtual ~StreamWriterBase();
public:
	virtual void SetFormat(const Encoder::Settings &settings) = 0;
	virtual void WriteMetatags(const FileTags &tags);
	virtual void WriteInterleaved(size_t count, const float *interleaved) = 0;
	virtual void WriteInterleavedConverted(size_t frameCount, const char *data);
	virtual void WriteCues(const std::vector<uint64> &cues);
	virtual void Finalize() = 0;
protected:
	void WriteBuffer();
};


//======================
class EncoderFactoryBase
//======================
{
private:
	Encoder::Traits traits;
protected:
	EncoderFactoryBase() { }
	virtual ~EncoderFactoryBase() { }
	void SetTraits(const Encoder::Traits &traits_);
public:
	virtual IAudioStreamEncoder *ConstructStreamEncoder(std::ostream &file) const = 0;
	const Encoder::Traits &GetTraits() const
	{
		return traits;
	}
	virtual mpt::ustring DescribeQuality(float quality) const;
	virtual mpt::ustring DescribeBitrateVBR(int bitrate) const;
	virtual mpt::ustring DescribeBitrateABR(int bitrate) const;
	virtual mpt::ustring DescribeBitrateCBR(int bitrate) const;
	virtual bool IsAvailable() const = 0;
};


OPENMPT_NAMESPACE_END
