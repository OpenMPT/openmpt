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

	struct Traits
	{
		
		std::string fileExtension;
		std::string fileDescription;
		std::string fileShortDescription;
		std::string encoderName;
		std::string description;

		bool canTags;
		std::vector<std::string> genres;
		
		int maxChannels;
		std::vector<uint32> samplerates;
		
		int modes;
		std::vector<int> bitrates;
		std::vector<Encoder::Format> formats;
		
		Encoder::Mode defaultMode;
		int defaultBitrate;
		float defaultQuality;
		int defaultFormat;

		Traits()
			: canTags(false)
			, maxChannels(0)
			, modes(Encoder::ModeInvalid)
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
		
		bool Tags;
		Encoder::Mode Mode;
		int Bitrate;
		float Quality;
		int Format;
		
		Settings(bool tags, Encoder::Mode mode, int bitrate, float quality, int format)
			: Tags(tags)
			, Mode(mode)
			, Bitrate(bitrate)
			, Quality(quality)
			, Format(format)
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
	virtual void SetFormat(int samplerate, int channels, const Encoder::Settings &settings) = 0;
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
	virtual void SetFormat(int samplerate, int channels, const Encoder::Settings &settings) = 0;
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
	virtual bool IsAvailable() const = 0;
};

