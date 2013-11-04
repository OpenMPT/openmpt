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

#include "soundlib/Tagging.h"
#include "soundlib/SampleFormat.h"

#include <iosfwd>
#include <string>
#include <vector>



static const int opus_bitrates [] = {
	8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 192, 224, 256, 320, 384, 448,      512
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
	192000, 176400, 128000,
	 96000,  88200,  64000,
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
		std::string Description;
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
		case Encoder::ModeCBR: return SettingValue("CBR", "Encoder::Mode"); break;
		case Encoder::ModeABR: return SettingValue("ABR", "Encoder::Mode"); break;
		case Encoder::ModeVBR: return SettingValue("VBR", "Encoder::Mode"); break;
		case Encoder::ModeQuality: return SettingValue("Quality", "Encoder::Mode"); break;
		case Encoder::ModeEnumerated: return SettingValue("Enumerated", "Encoder::Mode"); break;
		default: return SettingValue("Invalid", "Encoder::Mode"); break;
	}
}
template<> inline Encoder::Mode FromSettingValue(const SettingValue &val)
{
	ASSERT(val.GetTypeTag() == "Encoder::Mode");
	if(val.as<std::string>() == "") { return Encoder::ModeInvalid; }
	else if(val.as<std::string>() == "CBR") { return Encoder::ModeCBR; }
	else if(val.as<std::string>() == "ABR") { return Encoder::ModeABR; }
	else if(val.as<std::string>() == "VBR") { return Encoder::ModeVBR; }
	else if(val.as<std::string>() == "Quality") { return Encoder::ModeQuality; }
	else if(val.as<std::string>() == "Enumerated") { return Encoder::ModeEnumerated; }
	else { return Encoder::ModeInvalid; }
}

namespace Encoder
{

	struct Traits
	{
		
		std::string fileExtension;
		std::string fileDescription;
		std::string fileShortDescription;
		std::string encoderSettingsName;
		std::string encoderName;
		std::string description;

		bool canTags;
		std::vector<std::string> genres;
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
		
		Settings(SettingsContainer &conf, const std::string &encoderName, bool cues, bool tags, uint32 samplerate, uint16 channels, Encoder::Mode mode, int bitrate, float quality, int format)
			: Cues(conf, "Export", encoderName + "_" + "Cues", cues)
			, Tags(conf, "Export", encoderName + "_" + "Tags", tags)
			, Samplerate(conf, "Export", encoderName + "_" + "Samplerate", samplerate)
			, Channels(conf, "Export", encoderName + "_" + "Channels", channels)
			, Mode(conf, "Export", encoderName + "_" + "Mode", mode)
			, Bitrate(conf, "Export", encoderName + "_" + "Bitrate", bitrate)
			, Quality(conf, "Export", encoderName + "_" + "Quality", quality)
			, Format(conf, "Export", encoderName + "_" + "Format", format)
		{
			return;
		}

	};

} // namespace Encoder


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
	virtual std::string DescribeQuality(float quality) const;
	virtual std::string DescribeBitrateVBR(int bitrate) const;
	virtual std::string DescribeBitrateABR(int bitrate) const;
	virtual std::string DescribeBitrateCBR(int bitrate) const;
	virtual bool IsAvailable() const = 0;
};

