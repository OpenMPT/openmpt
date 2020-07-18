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
		uint32 Samplerate = 0;
		int Channels = 0;
		SampleFormat Sampleformat = SampleFormatInvalid;

		int Bitrate = 0;
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

	struct Traits
	{
		
		mpt::PathString fileExtension;
		mpt::ustring fileShortDescription;
		mpt::ustring encoderSettingsName;

		mpt::ustring fileDescription;

		bool canTags = false;
		std::vector<mpt::ustring> genres;
		int modesWithFixedGenres = 0;
		
		bool canCues = false;

		int maxChannels = 0;
		std::vector<uint32> samplerates;
		
		int modes = Encoder::ModeInvalid;
		std::vector<int> bitrates;
		std::vector<Encoder::Format> formats;
		
		uint32 defaultSamplerate = 48000;
		uint16 defaultChannels = 2;

		Encoder::Mode defaultMode = Encoder::ModeInvalid;
		int defaultBitrate = 0;
		float defaultQuality = 0.0f;
		int defaultFormat = 0;
		int defaultDitherType = 1;
	};

	struct StreamSettings
	{
		int32 FLACCompressionLevel = 5; // 8
		uint32 AUPaddingAlignHint = 4096;
		uint32 MP3ID3v2MinPadding = 1024;
		uint32 MP3ID3v2PaddingAlignHint = 4096;
		bool MP3ID3v2WriteReplayGainTXXX = true;
		int32 MP3LameQuality = 3; // 0
		bool MP3LameID3v2UseLame = false;
		bool MP3LameCalculateReplayGain = true;
		bool MP3LameCalculatePeakSample = true;
		int32 OpusComplexity = -1; // 10
	};

	struct Settings
	{

		bool Cues;
		bool Tags;

		uint32 Samplerate;
		uint16 Channels;

		Encoder::Mode Mode;
		int Bitrate;
		float Quality;
		int Format;
		int Dither;

		StreamSettings Details;

	};

} // namespace Encoder


class IAudioStreamEncoder
{
protected:
	IAudioStreamEncoder() { }
public:
	virtual ~IAudioStreamEncoder() = default;
public:
	virtual mpt::endian GetConvertedEndianness() const = 0;
	virtual void WriteInterleaved(size_t count, const float *interleaved) = 0;
	virtual void WriteInterleavedConverted(size_t frameCount, const std::byte *data) = 0;
	virtual void WriteCues(const std::vector<uint64> &cues) = 0; // optional
	virtual void WriteFinalize() = 0;
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
	mpt::endian GetConvertedEndianness() const override;
	void WriteInterleaved(size_t count, const float *interleaved) override = 0;
	void WriteInterleavedConverted(size_t frameCount, const std::byte *data) override;
	void WriteCues(const std::vector<uint64> &cues) override;
	void WriteFinalize() override;
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
