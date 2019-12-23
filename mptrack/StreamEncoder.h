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

#include "BuildSettings.h"

#include "Settings.h"

#include "../soundbase/SampleFormat.h"
#include "../common/Endianness.h"
#include "../soundlib/Tagging.h"

#include <iosfwd>
#include <string>
#include <vector>


OPENMPT_NAMESPACE_BEGIN



static constexpr int opus_bitrates [] = {
	8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 192, 224, 256, 320, 384, 448,      510
};
static constexpr int vorbis_bitrates [] = {
	           32,     48,     64, 80, 96, 112, 128,      160, 192, 224, 256, 320,           500
};
static constexpr int layer3_bitrates [] = {
	8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 192, 224, 256, 320
};
static constexpr int mpeg1layer3_bitrates [] = {
	           32, 40, 48, 56, 64, 80, 96, 112, 128,      160, 192, 224, 256, 320
};
static constexpr uint32 opus_samplerates [] = {
	48000,
	24000,           16000,
	12000,            8000
};
static constexpr uint32 opus_all_samplerates [] = {
	48000,  44100,  32000,
	24000,  22050,  16000,
	12000,  11025,   8000
};
static constexpr uint32 vorbis_samplerates [] = {
	 48000,  44100,  32000,
	 24000,  22050,  16000,
	 12000,  11025,   8000
};
static constexpr uint32 layer3_samplerates [] = {
	 48000,  44100,  32000,
	 24000,  22050,  16000
};
static constexpr uint32 mpeg1layer3_samplerates [] = {
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
		case Encoder::ModeCBR: return SettingValue(U_("CBR"), "Encoder::Mode"); break;
		case Encoder::ModeABR: return SettingValue(U_("ABR"), "Encoder::Mode"); break;
		case Encoder::ModeVBR: return SettingValue(U_("VBR"), "Encoder::Mode"); break;
		case Encoder::ModeQuality: return SettingValue(U_("Quality"), "Encoder::Mode"); break;
		case Encoder::ModeEnumerated: return SettingValue(U_("Enumerated"), "Encoder::Mode"); break;
		default: return SettingValue(U_("Invalid"), "Encoder::Mode"); break;
	}
}
template<> inline Encoder::Mode FromSettingValue(const SettingValue &val)
{
	ASSERT(val.GetTypeTag() == "Encoder::Mode");
	if(val.as<mpt::ustring>() == U_("")) { return Encoder::ModeInvalid; }
	else if(val.as<mpt::ustring>() == U_("CBR")) { return Encoder::ModeCBR; }
	else if(val.as<mpt::ustring>() == U_("ABR")) { return Encoder::ModeABR; }
	else if(val.as<mpt::ustring>() == U_("VBR")) { return Encoder::ModeVBR; }
	else if(val.as<mpt::ustring>() == U_("Quality")) { return Encoder::ModeQuality; }
	else if(val.as<mpt::ustring>() == U_("Enumerated")) { return Encoder::ModeEnumerated; }
	else { return Encoder::ModeInvalid; }
}

namespace Encoder
{

	struct Traits
	{
		
		mpt::PathString fileExtension;
		mpt::ustring fileShortDescription;
		mpt::ustring encoderSettingsName;

		mpt::ustring fileDescription;

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
			: canTags(false)
			, modesWithFixedGenres(0)
			, canCues(false)
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

	};

} // namespace Encoder


struct StreamEncoderSettings
{
	Setting<int32> FLACCompressionLevel;
	Setting<uint32> AUPaddingAlignHint;
	Setting<uint32> MP3ID3v2MinPadding;
	Setting<uint32> MP3ID3v2PaddingAlignHint;
	Setting<bool> MP3ID3v2WriteReplayGainTXXX;
	Setting<int32> MP3LameQuality;
	Setting<bool> MP3LameID3v2UseLame;
	Setting<bool> MP3LameCalculateReplayGain;
	Setting<bool> MP3LameCalculatePeakSample;
	Setting<int32> OpusComplexity;
	StreamEncoderSettings(SettingsContainer &conf, const mpt::ustring &section);
	static StreamEncoderSettings &Instance();
};


class IAudioStreamEncoder
{
protected:
	IAudioStreamEncoder() { }
public:
	virtual ~IAudioStreamEncoder() = default;
public:
	virtual mpt::endian GetConvertedEndianness() const = 0;
	virtual void WriteInterleaved(size_t count, const float *interleaved) = 0;
	virtual void WriteInterleavedConverted(size_t frameCount, const char *data) = 0;
	virtual void WriteCues(const std::vector<uint64> &cues) = 0; // optional
};


class StreamWriterBase
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
	virtual mpt::endian GetConvertedEndianness() const;
	virtual void WriteInterleaved(size_t count, const float *interleaved) = 0;
	virtual void WriteInterleavedConverted(size_t frameCount, const char *data);
	virtual void WriteCues(const std::vector<uint64> &cues);
protected:
	void WriteBuffer();
};


class EncoderFactoryBase
{
private:
	Encoder::Traits m_Traits;
protected:
	EncoderFactoryBase() { }
	virtual ~EncoderFactoryBase() = default;
	void SetTraits(const Encoder::Traits &traits);
public:
	virtual std::unique_ptr<IAudioStreamEncoder> ConstructStreamEncoder(std::ostream &file, const Encoder::Settings &settings, const FileTags &tags) const = 0;
	const Encoder::Traits &GetTraits() const
	{
		return m_Traits;
	}
	virtual bool IsBitrateSupported(int samplerate, int channels, int bitrate) const;
	virtual mpt::ustring DescribeQuality(float quality) const;
	virtual mpt::ustring DescribeBitrateVBR(int bitrate) const;
	virtual mpt::ustring DescribeBitrateABR(int bitrate) const;
	virtual mpt::ustring DescribeBitrateCBR(int bitrate) const;
	virtual bool IsAvailable() const = 0;
};


OPENMPT_NAMESPACE_END
